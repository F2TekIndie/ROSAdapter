
#pragma once

#include <filesystem>
#include <vector>
#include <string>



//
struct Configuration{
    /*
    */
    static Configuration LoadConfig( std::string const& sPath );

    //
    std::vector< std::string > _topics = {};
    //
    std::tuple< std::string, std::string > _cmdServerConnection = {"localhost","556"};
    std::tuple< std::string, std::string > _dataServerConnection = {"localhost","557"};
};
