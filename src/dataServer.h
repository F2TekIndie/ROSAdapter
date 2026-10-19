
#pragma once

#include "server.h"
#include <functional>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <map>



//
struct Subscribtion{};

//
class DataServer : public Server<DataServer> {
public:
    DataServer() = default;
    ~DataServer() = default;
    DataServer(const DataServer& rhs) = delete;
    //
    void HandleRequest( std::string const& sApiCall );

    //
    void CreateSubscription(std::string sTopic, std::string sType);

private:
    //
    std::map< std::string, std::shared_ptr<Subscribtion> > _Subscribtions = {};
};
