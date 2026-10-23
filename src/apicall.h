
#pragma once

#include <cstdint>
#include <string>



struct apiCall{
    std::string _sCommand = {""};
    bool        _local = {false};
    uint64_t    _requestId = {0};
};
