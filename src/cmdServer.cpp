
#include "cmdServer.h"



void CmdServer::start( Configuration const& conf ){
    _conf = conf;
}

bool CmdServer::exit() const{
    return _exit;
}