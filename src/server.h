
#pragma once

#include <boost/asio.hpp>
#include "configuration.h"



template<typename  T>
class Server{
public:
    Server() = default;
    virtual ~Server() = default;
    //
    void start( Configuration const& conf ){
        _conf = conf;
        // IP & PORT
        // Start listening for connections
    }
    //
    void stop(){
        // Close connections
        // Stop listening
    }
    //
    void send(){
        // 
        ;
    }
    //
    void receive(){
        // Send input to _derived->HandleRequest
        ;
    }

private:
    Configuration _conf = {};
    T* _derived = {(T*)this};
};
