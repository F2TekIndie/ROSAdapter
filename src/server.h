#pragma once

#include "adapter.grpc.pb.h"
#include "rosdata/converter.h"
#include <rclcpp/rclcpp.hpp>
#include <map>
#include <mutex>

//
class AdapterService final : public robotics::ROSAdapter::Service{
public:
    explicit AdapterService( rclcpp::Node::SharedPtr node );
    //
    grpc::Status ListTopics( grpc::ServerContext*, const robotics::ListTopicsRequest*, robotics::ListTopicsResponse* ) override;
    grpc::Status Subscribe( grpc::ServerContext*, const robotics::SubscribeRequest*, robotics::Topic* ) override;
    grpc::Status Unsubscribe( grpc::ServerContext*, const robotics::TopicRequest*, robotics::Topic* ) override;
    grpc::Status ListSupportedTypes( grpc::ServerContext*, const robotics::ListSupportedTypesRequest*, robotics::ListSupportedTypesResponse* ) override;
    grpc::Status GetSchema( grpc::ServerContext*, const robotics::SchemaRequest*, robotics::SchemaResponse* ) override;
    grpc::Status GetLatest( grpc::ServerContext*, const robotics::TopicRequest*, robotics::LatestData* ) override;

private:
    struct Entry{
        robotics::Topic topic;
        robotics::LatestData latest;
        std::shared_ptr<rosdata::Converter> converter;
        rclcpp::SubscriptionBase::SharedPtr subscription;
    };

    void refreshTopics(); // Called with mutex held.
    rosdata::SchemaRegistry schemas_;
    rclcpp::Node::SharedPtr node_;
    std::mutex mutex_;
    std::map<std::string, std::shared_ptr<Entry>> topics_;
};
