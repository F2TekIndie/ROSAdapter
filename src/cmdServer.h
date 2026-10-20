
#pragma once

#include <boost/asio.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/io_context.hpp>
#include <map>


//
typedef typename boost::asio::experimental::concurrent_channel<void(boost::system::error_code, std::string)> MyChannelType;

//
struct Subscribtion{};

//
class CmdServer{
public:
    explicit CmdServer(boost::asio::io_context& ctx);
    ~CmdServer() = default;
    CmdServer(const CmdServer& rhs) = delete;
    //
    bool exit() const;
    void HandleRequest( std::string const& sApiCall );
    //
    void CreateSubscription( std::string sTopic, std::string sType );
    //
    MyChannelType& cmdChannel();
    MyChannelType& dataChannel();

private:
    //
    void TopicsQuery();
    //
    bool            _exit = {false};
    std::map< std::string, std::shared_ptr<Subscribtion> > _Subscribtions = {};
    MyChannelType _cmdChannel;
    MyChannelType _dataChannel;
};
