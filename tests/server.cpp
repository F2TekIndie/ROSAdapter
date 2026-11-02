#include "server.h"
#include "ros_messages.pb.h"
#include <grpcpp/grpcpp.h>
#include <sensor_msgs/msg/image.hpp>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>



//
namespace{
    void require( bool condition, const std::string& message ){
        if ( !condition ){
            throw std::runtime_error( message );
        }
    }

    struct RunningServer{
        rclcpp::Node::SharedPtr node = std::make_shared<rclcpp::Node>( "rosdata_grpc_test" );
        AdapterService service{ node };
        rclcpp::executors::SingleThreadedExecutor executor;
        std::unique_ptr<grpc::Server> server;
        std::unique_ptr<robotics::ROSAdapter::Stub> client;
        std::thread worker;

        RunningServer(){
            grpc::ServerBuilder builder;
            builder.SetMaxSendMessageSize( 72 * 1024 * 1024 );
            builder.SetMaxReceiveMessageSize( 72 * 1024 * 1024 );
            int port = 0;
            builder.AddListeningPort( "127.0.0.1:0", grpc::InsecureServerCredentials(), &port );
            builder.RegisterService( &service );
            server = builder.BuildAndStart();
            require( static_cast<bool>( server ), "Cannot start test server" );
            grpc::ChannelArguments arguments;
            arguments.SetMaxReceiveMessageSize( 72 * 1024 * 1024 );
            client = robotics::ROSAdapter::NewStub( grpc::CreateCustomChannel(
                "127.0.0.1:" + std::to_string( port ), grpc::InsecureChannelCredentials(), arguments ) );
            executor.add_node( node );
            worker = std::thread( [this] {
                executor.spin();
            } );
        }

        ~RunningServer(){
            server->Shutdown();
            server->Wait();
            executor.cancel();
            worker.join();
        }
    };

    void deadline( grpc::ClientContext& context ){
        context.set_deadline( std::chrono::system_clock::now() + std::chrono::seconds( 3 ) );
    }
} // namespace

//
int main( int argc, char** argv ){
    rclcpp::init( argc, argv );
    int result = 0;
    try{
        RunningServer running;
        robotics::ListSupportedTypesRequest typesRequest;
        robotics::ListSupportedTypesResponse types;
        grpc::ClientContext typesContext;
        deadline( typesContext );
        require( running.client->ListSupportedTypes( &typesContext, typesRequest, &types ).ok() && types.types_size() >= 143,
            "Supported types RPC failed" );

        robotics::SchemaRequest schemaRequest;
        robotics::SchemaResponse schema;
        schemaRequest.set_type( "sensor_msgs/msg/Image" );
        grpc::ClientContext schemaContext;
        deadline( schemaContext );
        require( running.client->GetSchema( &schemaContext, schemaRequest, &schema ).ok(), "Schema RPC failed" );
        rosdata::SchemaRegistry schemas;
        schemas.loadSchema( schema.descriptors() );

        const std::string topic = "/rosdata_converter_test/image";
        auto publisher = running.node->create_publisher<sensor_msgs::msg::Image>( topic, 1 );
        robotics::SubscribeRequest subscription;
        subscription.set_topic( topic );
        subscription.set_type( "sensor_msgs/msg/Image" );
        robotics::Topic state;
        grpc::ClientContext subscribeContext;
        deadline( subscribeContext );
        require( running.client->Subscribe( &subscribeContext, subscription, &state ).ok() && state.subscribed(), "Image subscription failed" );
        grpc::ClientContext repeatContext;
        deadline( repeatContext );
        require( running.client->Subscribe( &repeatContext, subscription, &state ).ok(), "Repeated subscription failed" );

        robotics::TopicRequest request;
        request.set_topic( topic );
        robotics::LatestData latest;
        grpc::ClientContext emptyContext;
        deadline( emptyContext );
        require( running.client->GetLatest( &emptyContext, request, &latest ).ok() && !latest.has_payload(), "Initial cache was not empty" );

        sensor_msgs::msg::Image image;
        image.header.frame_id = "test_camera";
        image.width = 2048;
        image.height = 1024;
        image.encoding = "rgb8";
        image.step = image.width * 3;
        image.data.resize( image.step * image.height, 0xff );
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 8 );
        while ( std::chrono::steady_clock::now() < until ){
            publisher->publish( image );
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            grpc::ClientContext latestContext;
            deadline( latestContext );
            require( running.client->GetLatest( &latestContext, request, &latest ).ok(), "Latest-data RPC failed" );
            if ( latest.has_payload() ){
                break;
            }
        }
        require( latest.has_payload() && latest.sequence() > 0 && latest.conversion_error().empty(), "Image did not reach the cache" );
        robotics::ros::sensor_msgs__msg__Image converted;
        require( latest.payload().UnpackTo( &converted ), "Wrong image protobuf type" );
        require( converted.data().size() == image.data.size() && static_cast<unsigned char>( converted.data().back() ) == 255,
            "Large binary image was corrupted over gRPC" );
        require( converted.header().frame_id() == image.header.frame_id, "Nested image header was lost" );

        const auto sequence = latest.sequence();
        image.header.frame_id = "updated_camera";
        const auto updateUntil = std::chrono::steady_clock::now() + std::chrono::seconds( 5 );
        while ( std::chrono::steady_clock::now() < updateUntil ){
            publisher->publish( image );
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            grpc::ClientContext latestContext;
            deadline( latestContext );
            running.client->GetLatest( &latestContext, request, &latest );
            latest.payload().UnpackTo( &converted );
            if ( latest.sequence() > sequence && converted.header().frame_id() == image.header.frame_id ){
                break;
            }
        }
        require( latest.sequence() > sequence && converted.header().frame_id() == image.header.frame_id, "Latest cache was not replaced" );

        grpc::ClientContext unsubscribeContext;
        deadline( unsubscribeContext );
        require( running.client->Unsubscribe( &unsubscribeContext, request, &state ).ok() && !state.subscribed(), "Unsubscribe failed" );
        grpc::ClientContext removedContext;
        deadline( removedContext );
        require( running.client->GetLatest( &removedContext, request, &latest ).error_code() == grpc::StatusCode::FAILED_PRECONDITION,
            "Unsubscribed cache remained readable" );
    }
    catch ( const std::exception& error ){
        std::cerr << error.what() << std::endl;
        result = 1;
    }
    rclcpp::shutdown();
    return result;
}
