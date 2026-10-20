
#include "server.h"
#include "cmdServer.h"
#include <iostream>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>



//
namespace{
    using namespace boost::asio::experimental::awaitable_operators;

    // --- Reader: pulls from socket, pushes to channel ---
    boost::asio::awaitable<void> reader(boost::asio::ip::tcp::socket& sock, MyChannelType& ch) {
        try {
            boost::asio::streambuf buf;
            while (true) {
                buf.consume(buf.size());
                auto [ec, n] = co_await boost::asio::async_read_until( sock, buf, '\n', boost::asio::as_tuple(boost::asio::use_awaitable) );
                if ( ec )
                    break;
                auto line = std::string(boost::asio::buffer_cast<const char*>(buf.data()), n);
                co_await ch.async_send(boost::system::error_code{},std::move(line), boost::asio::use_awaitable);
            }
        } catch (const std::exception&) { /* connection closed */ }
        ch.close(); // signal writer to stop
    }

    // --- Writer: pulls from channel, pushes to socket ---
    boost::asio::awaitable<void> writer(boost::asio::ip::tcp::socket& sock, MyChannelType& ch) {
        try {
            while (true){
                auto msg = co_await ch.async_receive(boost::asio::use_awaitable);
                // Transform or just echo
                co_await boost::asio::async_write(sock, boost::asio::buffer(msg), boost::asio::use_awaitable);
            }
        } catch (const std::exception&) { /* connection closed */ }
    }

    boost::asio::awaitable<void> session(boost::asio::ip::tcp::socket socket, CmdServer& server ) {
        auto& cmdCh = server.cmdChannel();
        auto& dataCh = server.dataChannel();

        // Run reader and writer in parallel; await BOTH completing
        co_await (reader(socket, cmdCh) && writer(socket, dataCh));

        socket.close();
    }

    boost::asio::awaitable<void> listener(boost::asio::io_context& ctx, unsigned short port, CmdServer& server ) {
        boost::asio::ip::tcp::acceptor acc(ctx, {boost::asio::ip::tcp::v4(), port});
        while (true) {
            boost::asio::ip::tcp::socket socket = co_await acc.async_accept(boost::asio::use_awaitable);
            boost::asio::co_spawn(ctx, session(std::move(socket), server), boost::asio::detached);
        }
    }

    void runAsioCommunication( boost::asio::io_context& ctx ){
        // Move to thread
        ctx.run();
    }
    void receiveCommands( CmdServer& server ){
        auto& cmdCh = server.cmdChannel();
        cmdCh.async_receive(
            [&](boost::system::error_code ec, std::string sMsg){
                if ( !ec ){
                    // Messages are terminated with new line.
                    // Needs to be stripped for valid command.
                    server.HandleRequest( {sMsg.substr(0, sMsg.size()-1), false} );
                }
                if ( !server.exit() ){
                    receiveCommands( server );
                }
        });
    }
}

namespace Server {
    std::jthread tWorker;
    std::jthread tReceiverParser;
    //
    void start( boost::asio::io_context& ctx, Configuration const& conf, CmdServer& server ){
        // IP & PORT
        // Start listening for connections
        auto listen = listener(ctx, 8080, server);
        boost::asio::co_spawn(ctx, std::move(listen), boost::asio::detached);
        
        tWorker = std::jthread( runAsioCommunication, std::ref(ctx ) );
        tReceiverParser = std::jthread( receiveCommands, std::ref(server) );
    }
    //
    void stop( boost::asio::io_context& ctx ){
        // Close connections
        // Stop listening
        ctx.stop();
        tWorker.join();
        tReceiverParser.join();
    }
    void sendMessage( CmdServer& server, std::string sMessage ){
        auto& dataCh = server.dataChannel();
        sMessage += "\n";
        dataCh.async_send(boost::system::error_code {}, std::move(sMessage), [](boost::system::error_code ec){
                if ( ec ){
                    std::cout << "Error while sending message: " << ec << std::endl;
                }
        });
    }
}
