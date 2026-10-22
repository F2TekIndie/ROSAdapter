
#include "server.h"
#include "cmdServer.h"
#include <iostream>
#include <stdexcept>
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
            while (true) {
                std::array<unsigned char, 4> header;
                co_await boost::asio::async_read( sock, boost::asio::buffer(header), boost::asio::use_awaitable );
                //
                std::uint32_t size = (std::uint32_t(header[0])<<24) | (std::uint32_t(header[1])<<16) | (std::uint32_t(header[2])<<8) | std::uint32_t(header[3]);
                constexpr std::uint32_t maxSize = 16 * 1024 * 1024;
                if ( size > maxSize ){
                    throw std::runtime_error("Message to long");
                }
                //
                std::string sPayload(size,'\0');
                co_await boost::asio::async_read( sock, boost::asio::buffer(sPayload), boost::asio::use_awaitable );
                //
                co_await ch.async_send( boost::system::error_code{}, std::move(sPayload), boost::asio::use_awaitable );
            }
        } catch (const std::exception&) { /* connection closed */ }
        ch.close(); // signal writer to stop
    }

    // --- Writer: pulls from channel, pushes to socket ---
    boost::asio::awaitable<void> writer(boost::asio::ip::tcp::socket& sock, MyChannelType& ch) {
        try {
            while (true){
                auto msg = co_await ch.async_receive(boost::asio::use_awaitable);
                constexpr std::size_t maxSize = 16*1024*1024;
                if ( msg.size() > maxSize ){
                    throw std::runtime_error("Message to long");
                }
                auto size = static_cast<std::uint32_t>(msg.size());
                std::array<unsigned char, 4> header{
                    static_cast<unsigned char>(size >> 24),
                    static_cast<unsigned char>(size >> 16),
                    static_cast<unsigned char>(size >> 8),
                    static_cast<unsigned char>(size)
                };
                std::array<boost::asio::const_buffer, 2> buffers{
                    boost::asio::buffer(header),
                    boost::asio::buffer(msg)
                };
                // Transform or just echo
                co_await boost::asio::async_write(sock, buffers, boost::asio::use_awaitable);
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
                    server.HandleRequest( {sMsg, false} );
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
        dataCh.async_send(boost::system::error_code {}, std::move(sMessage), [](boost::system::error_code ec){
                if ( ec ){
                    std::cout << "Error while sending message: " << ec << std::endl;
                }
        });
    }
}
