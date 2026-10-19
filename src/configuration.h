
#pragma once

#include <filesystem>
#include <vector>
#include <string>



//
class Configuration{
public:
    static Configuration LoadConfig( std::string const& sPath );

private:
    std::vector< std::string > topics_;
};
