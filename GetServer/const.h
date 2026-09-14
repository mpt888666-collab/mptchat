//
// Created by mpt on 2026/6/30.
//
#ifndef GETSERVER_CONST_H
#define GETSERVER_CONST_H
#include <map>
#include <string>
#include <iostream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include "ConfigMgr.h"
#include "Singleton.h"
#include "RedisMgr.h"
#include <queue>
enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,  //Json解析错误
    RPCFailed = 1002,  //RPC请求错误
    VarifyExpired = 1003,
    VarifyCodeErr = 1004,
    UserAlreadyExist = 1005,
    RegisterFailed = 1006,
    UserNotExist = 1007,
    ModifyPassErr = 1008,
    ModifyLoginErr = 1009,
    RPCGetFailed = 1010,
};

class ConfigMgr;
extern ConfigMgr gCfgMgr;
#endif //GETSERVER_CONST_H