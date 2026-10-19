
#pragma once

#include <boost/asio.hpp>
#include "configuration.h"



class DataServer{
public:
    DataServer() = default;
    ~DataServer() = default;
    DataServer(const DataServer& rhs) = delete;

    //
    void start( Configuration const& conf );

private:
    Configuration _conf = {};

};
