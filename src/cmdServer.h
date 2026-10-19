
#pragma once

#include "server.h"



class CmdServer : public Server<CmdServer>{
public:
    CmdServer() = default;
    ~CmdServer() = default;
    CmdServer(const CmdServer& rhs) = delete;
    //
    bool exit() const;
    //
    void HandleRequest( std::string const& sApiCall );

private:
    //
    void TopicsQuery();
    //
    bool            _exit = {false};
};
