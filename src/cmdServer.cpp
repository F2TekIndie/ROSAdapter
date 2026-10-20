
#include "cmdServer.h"
#include "apicall.h"
#include "server.h"
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int32.hpp>
#include <iostream>
#include <map>
//
using std::placeholders::_1;



//
namespace{
    const std::string API_TOPICS_QUERY  = "TopicsQuery";
    const std::string API_EXIT          = "Exit";
    const std::string API_LIST          = "List";
    //
    const std::string TYPE_STRING   ="String";
    const std::string TYPE_INT      ="Int";
    //
    template<typename T>
    struct TypedSubscribtion : public Subscribtion{
        typedef typename rclcpp::Subscription< T >::SharedPtr MyPtr;
        //
        explicit TypedSubscribtion( std::string sTopic )
            :_node("mini_sub")
        {
            _subscription = _node.create_subscription< T >( sTopic, 10, std::bind(&TypedSubscribtion::callback, this, _1) );
        }
        //
        void callback(const typename T::SharedPtr msg) const{
            std::cout << "I heard something." << std::endl;
        }
        //
        rclcpp::Node _node;
        MyPtr _subscription;
        ////
        TypedSubscribtion() = delete;
        TypedSubscribtion(const TypedSubscribtion& rhs) = delete;
    };
}


//
CmdServer::CmdServer(boost::asio::io_context& ctx)
    :_cmdChannel( ctx.get_executor() )
    ,_dataChannel( ctx.get_executor() )
{}

MyChannelType& CmdServer::cmdChannel(){
    return _cmdChannel;
}
MyChannelType& CmdServer::dataChannel(){
    return _dataChannel;
}

//
bool CmdServer::exit() const{
    return _exit;
}

void CmdServer::HandleRequest( apiCall call ){
    if ( call._sCommand == API_TOPICS_QUERY ){
        TopicsQuery( call );
    }
    else if ( call._sCommand == API_EXIT ){
        _exit = true;
    }
    else if ( call._sCommand == API_LIST ){
        std::cout << API_EXIT << " | " << API_LIST << " | "<< API_TOPICS_QUERY << std::endl;
    }
    else{
        std::cout << "Unknow command!" << std::endl;
    }
}

/*
*/
void CmdServer::TopicsQuery( apiCall call ){
    //
    rclcpp::Node testNode("topic_query_node");
    //
    std::map<std::string, std::vector<std::string> > topics = testNode.get_topic_names_and_types();
    //
    if ( call._local ){
        for ( const auto& topic_it : topics ){
            std::cout << "Topic:  " << topic_it.first;
            std::cout << " | Type: ";
            for ( const auto& type : topic_it.second ){
                std::cout << type << " ";
            }
            std::cout << std::endl;
        }
    }
    else{
        // Prepare response message
        std::string sMessage = {"Just a dummy answer for now!"};
        Server::sendMessage( *this, sMessage);
    }
}
//
void CmdServer::CreateSubscription(std::string sTopic, std::string sType){
    if ( sType == TYPE_STRING ){
        std::shared_ptr<Subscribtion> pSub( new TypedSubscribtion<std_msgs::msg::String>( sTopic ) );
        _Subscribtions[sTopic] = pSub;
    }
    else if ( sType == TYPE_INT ){
        std::shared_ptr<Subscribtion> pSub( new TypedSubscribtion<std_msgs::msg::Int32>( sTopic ) );
        _Subscribtions[sTopic] = pSub;
    }
}