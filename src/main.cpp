#include "server.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <stdexcept>



//
int main( int argc, char* argv[] ){
    rclcpp::init( argc, argv );
    int result = 0;
    try{
        auto node = std::make_shared<rclcpp::Node>( "rosadapter" );
        AdapterService service( node );
        grpc::ServerBuilder builder;
        builder.SetMaxReceiveMessageSize( 72 * 1024 * 1024 );
        builder.SetMaxSendMessageSize( 72 * 1024 * 1024 );
        const std::string address = node->declare_parameter<std::string>( "grpc_address", "127.0.0.1:8080" );
        builder.AddListeningPort( address, grpc::InsecureServerCredentials() );
        builder.RegisterService( &service );
        auto server = builder.BuildAndStart();
        if ( !server ){
            throw std::runtime_error( "Cannot listen on " + address );
        }
        std::cout << "ROSAdapter listening on " << address << std::endl;
        rclcpp::executors::SingleThreadedExecutor executor;
        executor.add_node( node );
        try{
            executor.spin();
        }
        catch ( ... ){
            server->Shutdown();
            server->Wait();
            throw;
        }
        server->Shutdown();
        server->Wait();
    }
    catch ( const std::exception& error ){
        std::cerr << error.what() << std::endl;
        result = 1;
    }
    rclcpp::shutdown();
    return result;
}
