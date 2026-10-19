
#pragma once

#include <boost/asio.hpp>
#include "configuration.h"



class CmdServer{
public:
    CmdServer() = default;
    ~CmdServer() = default;
    CmdServer(const CmdServer& rhs) = delete;
    
    //
    void start( Configuration const& conf );
    //
    bool exit() const;

private:
    Configuration   _conf = {};
    bool            _exit = {false};
};
