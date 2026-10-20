

#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/detail/error_code.hpp>
#include <csignal>
#include <exception>
#include <iostream>
#include <istream>
#include <rclcpp/rclcpp.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <system_error>
#include <thread>
#include <unistd.h>
#include "configuration.h"
#include "cmdServer.h"
#include "server.h"



namespace{
    //
    std::atomic<bool> _exitAdapter = false;
    //
    void signalExitHandler(int nSig){
        std::cout << "Exit handler called. Gracefully closing down." << std::endl;
        _exitAdapter = true;
    }
    //
    boost::asio::awaitable<void> readConsole( CmdServer& server ){
        namespace asio =  boost::asio;
        auto executor = co_await asio::this_coro::executor;
        int fd = ::dup(STDIN_FILENO);
        if ( fd == -1 ){
            throw std::system_error(errno, std::generic_category(), "dup stdin");
        }

        asio::posix::stream_descriptor input(executor, fd);
        asio::streambuf buffer;
        while ( !server.exit() ){
            std::cout << "Enter command: " << std::endl;
            boost::system::error_code ec;
            co_await asio::async_read_until( input, buffer, '\n', asio::redirect_error(asio::use_awaitable, ec) );
            if ( ec ){
                co_return;
            }
            //
            std::istream stream( &buffer );
            std::string sCommand;
            std::getline( stream, sCommand );
            //
            if ( !sCommand.empty() && sCommand.back() == '\r' ){
                sCommand.pop_back();
            }
            if ( !sCommand.empty() ){
                server.HandleRequest( {sCommand, true} );
            }
        }
    }
}
//
int main(int argc, char* argv[]){
    std::cout << "I can start'" << std::endl;
    // Parse commandline args
    if ( argc != 2 ){
        std::cout << "Wrong number of commandline arguments." << std::endl;
        return -1;
    }
    // 
    rclcpp::init( argc, argv );
    ::signal( SIGINT, signalExitHandler );
    boost::asio::io_context ctx;
    // Load configuration
    std::string sConfigName = argv[1];
    Configuration conf = Configuration::LoadConfig( sConfigName );
    // Start command server
    CmdServer _cmdServer( ctx );
    Server::start( ctx, conf, _cmdServer );
    boost::asio::co_spawn( ctx, readConsole( _cmdServer ), [&ctx](std::exception_ptr error){} );
    // Loop until command server quits or sighandler is called
    while ( !_cmdServer.exit() && !_exitAdapter ){
        std::this_thread::sleep_for( std::chrono::seconds(1) );
    }
    //
    Server::stop( ctx );
    //
    rclcpp::shutdown();
    return 0;
}
