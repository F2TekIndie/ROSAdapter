
#pragma once

#include "configuration.h"
#include <boost/asio/io_context.hpp>



//
class CmdServer;

//
namespace Server{
    void start( boost::asio::io_context& ctx, Configuration const& conf, CmdServer& server );
    void stop( boost::asio::io_context& ctx );
    void sendMessage( CmdServer& server, std::string sMessage );
};
