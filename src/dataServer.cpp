
#include "dataServer.h"
#include <std_msgs/msg/string.hpp>
#include <std_msgs/msg/int32.hpp>
//
using std::placeholders::_1;


//
namespace{
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
void DataServer::HandleRequest( std::string const& sApiCall ){
    // We should only transmit data. No command handling
}


//
void DataServer::CreateSubscription(std::string sTopic, std::string sType){
    if ( sType == TYPE_STRING ){
        std::shared_ptr<Subscribtion> pSub( new TypedSubscribtion<std_msgs::msg::String>( sTopic ) );
        _Subscribtions[sTopic] = pSub;
    }
    else if ( sType == TYPE_INT ){
        std::shared_ptr<Subscribtion> pSub( new TypedSubscribtion<std_msgs::msg::Int32>( sTopic ) );
        _Subscribtions[sTopic] = pSub;
    }
}
