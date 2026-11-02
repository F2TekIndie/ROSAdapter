#include "server.h"
#include <algorithm>
#include <chrono>



//
namespace{
    template <typename EntryType>
    void update( std::mutex& mutex, const std::weak_ptr<EntryType>& weak, const rosdata::Converter& converter, const rclcpp::SerializedMessage& message ){
        google::protobuf::Any payload;
        std::string error;
        try{
            payload = converter.toProtobuf( message );
        }
        catch ( const std::exception& exception ){
            error = exception.what();
        }
        // Conversion is outside the registry lock. Discard stale callbacks after unsubscribe.
        std::lock_guard lock( mutex );
        auto entry = weak.lock();
        if ( !entry || !entry->subscription ){
            return;
        }
        auto& latest = entry->latest;
        latest.set_conversion_error( error );
        if ( !error.empty() ){
            return;
        }
        *latest.mutable_payload() = std::move( payload );
        latest.set_sequence( latest.sequence() + 1 );
        latest.set_received_unix_nanoseconds( std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch() )
                .count() );
    }
} // namespace

//
AdapterService::AdapterService( rclcpp::Node::SharedPtr node )
    :node_( std::move( node ) )
{
}

//
void AdapterService::refreshTopics(){
    const auto discovered = node_->get_topic_names_and_types();
    const auto supported = schemas_.supportedTypes();
    for ( auto it = topics_.begin(); it != topics_.end(); ){
        if ( !it->second->subscription && !discovered.contains( it->first ) ){
            it = topics_.erase( it );
        }
        else{
            it->second->topic.clear_supported_types();
            it->second->topic.clear_types();
            ++it;
        }
    }
    for ( const auto& [name, types] : discovered ){
        auto& entry = topics_[name];
        if ( !entry ){
            entry = std::make_shared<Entry>();
        }
        entry->topic.set_name( name );
        for ( const auto& type : types ){
            entry->topic.add_types( type );
            if ( std::find( supported.begin(), supported.end(), type ) != supported.end() ){
                entry->topic.add_supported_types( type );
            }
        }
    }
}

//
grpc::Status AdapterService::ListTopics( grpc::ServerContext*, const robotics::ListTopicsRequest*, robotics::ListTopicsResponse* response ){
    std::lock_guard lock( mutex_ );
    refreshTopics();
    for ( const auto& [name, entry] : topics_ ){
        *response->add_topics() = entry->topic;
    }
    return grpc::Status::OK;
}

//
grpc::Status AdapterService::Subscribe( grpc::ServerContext*, const robotics::SubscribeRequest* request, robotics::Topic* response ){
    if ( request->topic().empty() || request->type().empty() ){
        return { grpc::StatusCode::INVALID_ARGUMENT, "Topic and type are required" };
    }
    std::lock_guard lock( mutex_ );
    refreshTopics();
    auto found = topics_.find( request->topic() );
    if ( found == topics_.end() ){
        return { grpc::StatusCode::NOT_FOUND, "Topic is not advertised" };
    }
    auto& entry = found->second;
    if ( entry->subscription ){
        if ( entry->topic.subscribed_type() != request->type() ){
            return { grpc::StatusCode::FAILED_PRECONDITION, "Already subscribed with another type" };
        }
        *response = entry->topic;
        return grpc::Status::OK;
    }
    const auto& types = entry->topic.types();
    if ( std::find( types.begin(), types.end(), request->type() ) == types.end() ){
        return { grpc::StatusCode::INVALID_ARGUMENT, "Type is not advertised for this topic" };
    }
    // Weak ownership prevents a cycle and rejects callbacks from a removed subscription.
    std::weak_ptr<Entry> weak = entry;

    const auto supported = schemas_.supportedTypes();
    if ( std::find( supported.begin(), supported.end(), request->type() ) == supported.end() ){
        return { grpc::StatusCode::UNIMPLEMENTED, "No generated protobuf schema for this ROS type" };
    }
    try{
        auto converter = std::make_shared<rosdata::Converter>( request->type(), schemas_ );
        entry->subscription = node_->create_generic_subscription( request->topic(), request->type(), rclcpp::SensorDataQoS().keep_last( 1 ),
            [this, weak, converter]( std::shared_ptr<rclcpp::SerializedMessage> message ){
            update( mutex_, weak, *converter, *message );
        } );
        entry->converter = std::move( converter );
    }
    catch ( const std::exception& error ){
        return { grpc::StatusCode::FAILED_PRECONDITION, error.what() };
    }
    entry->topic.set_subscribed( true );
    entry->topic.set_subscribed_type( request->type() );
    entry->latest.set_topic( request->topic() );
    entry->latest.set_type( request->type() );
    *response = entry->topic;
    return grpc::Status::OK;
}

//
grpc::Status AdapterService::Unsubscribe( grpc::ServerContext*, const robotics::TopicRequest* request, robotics::Topic* response ){
    std::lock_guard lock( mutex_ );
    auto found = topics_.find( request->topic() );
    if ( found == topics_.end() ){
        return { grpc::StatusCode::NOT_FOUND, "Unknown topic" };
    }
    auto replacement = std::make_shared<Entry>();
    replacement->topic = found->second->topic;
    replacement->topic.set_subscribed( false );
    replacement->topic.clear_subscribed_type();
    found->second = replacement;
    *response = replacement->topic;
    return grpc::Status::OK;
}

//
grpc::Status AdapterService::GetLatest( grpc::ServerContext*, const robotics::TopicRequest* request, robotics::LatestData* response ){
    std::lock_guard lock( mutex_ );
    auto found = topics_.find( request->topic() );
    if ( found == topics_.end() ){
        return { grpc::StatusCode::NOT_FOUND, "Unknown topic" };
    }
    if ( !found->second->subscription ){
        return { grpc::StatusCode::FAILED_PRECONDITION, "Topic is not subscribed" };
    }
    *response = found->second->latest;
    return grpc::Status::OK;
}

//
grpc::Status AdapterService::ListSupportedTypes( grpc::ServerContext*, const robotics::ListSupportedTypesRequest*, robotics::ListSupportedTypesResponse* response ){
    for ( const auto& type : schemas_.supportedTypes() ){
        response->add_types( type );
    }
    return grpc::Status::OK;
}

//
grpc::Status AdapterService::GetSchema( grpc::ServerContext*, const robotics::SchemaRequest* request, robotics::SchemaResponse* response ){
    try{
        response->set_type( request->type() );
        response->set_protobuf_type( schemas_.descriptor( request->type() )->full_name() );
        *response->mutable_descriptors() = schemas_.schema( request->type() );
        return grpc::Status::OK;
    }
    catch ( const std::invalid_argument& error ){
        return { grpc::StatusCode::NOT_FOUND, error.what() };
    }
}
