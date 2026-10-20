

#include <csignal>
#include <iostream>
#include <rclcpp/rclcpp.hpp>
#include <boost/asio.hpp>
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
    // Start command server => Starts its own thread
    CmdServer _cmdServer( ctx );
    Server::start( ctx, conf, _cmdServer );
    
    // Loop until command server quits or sighandler is called
    while ( !_cmdServer.exit() && !_exitAdapter ){
        std::cout << std::endl << "Enter command: ";
        //
        std::string sCmd;
        std::cin >> sCmd;
        //
        _cmdServer.HandleRequest( sCmd );
    }
    //
    Server::stop( ctx );
    //
    rclcpp::shutdown();
    return 0;
}
