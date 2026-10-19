
#pragma once

#include "server.h"



class DataServer : public Server<DataServer> {
public:
    DataServer() = default;
    ~DataServer() = default;
    DataServer(const DataServer& rhs) = delete;
    //
    void HandleRequest( std::string const& sApiCall );

private:
};
