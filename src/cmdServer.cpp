
#include "cmdServer.h"
#include <rclcpp/rclcpp.hpp>
#include <iostream>
#include <map>



//
namespace{
    const std::string API_TOPICS_QUERY  = "TopicsQuery";
    const std::string API_EXIT          = "Exit";
    const std::string API_LIST          = "List";
}


//
bool CmdServer::exit() const{
    return _exit;
}

void CmdServer::HandleRequest( std::string const& sApiCall ){
    if ( sApiCall == API_TOPICS_QUERY ){
        TopicsQuery();
    }
    else if ( sApiCall == API_EXIT ){
        _exit = true;
    }
    else if ( sApiCall == API_LIST ){
        std::cout << API_EXIT << " | " << API_LIST << " | "<< API_TOPICS_QUERY << std::endl;
    }
    else{
        std::cout << "Unknow command!" << std::endl;
    }
}

/*
*/
void CmdServer::TopicsQuery(){
    //
    rclcpp::Node testNode("topic_query_node");
    //
    std::map<std::string, std::vector<std::string> > topics = testNode.get_topic_names_and_types();
    //
    for ( const auto& topic_it : topics ){
        std::cout << "Topic:  " << topic_it.first;
        std::cout << " | Type: ";
        for ( const auto& type : topic_it.second ){
            std::cout << type << " ";
        }
        std::cout << std::endl;
    }
}