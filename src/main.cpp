

#include <csignal>
#include <iostream>
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include "configuration.h"
#include "cmdServer.h"
#include "dataServer.h"


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
    ::signal( SIGINT, signalExitHandler );
    // Load configuration
    std::string sConfigName;
    Configuration conf = Configuration::LoadConfig( sConfigName );
    // Start command server => Starts its own thread
    CmdServer _cmdServer;
    _cmdServer.start( conf );
    // Start data server => Starts its own thread
    DataServer _dataServer;
    _dataServer.start( conf );
    
    // Loop until command server quits or sighandler is called
    while ( !_cmdServer.exit() && !_exitAdapter ){
        // Main thread should be idle
        sleep( 1 );
    }
    return 0;
}
