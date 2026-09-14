//
// Created by mpt on 2026/8/23.
//

#ifndef RESOURCESERVER_CONST_H
#define RESOURCESERVER_CONST_H
#include <map>
#include <string>
#include <iostream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include "ConfigMgr.h"
#include "Singleton.h"
#include <queue>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#define HEAD_TOTAL_LEN 6
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 4
#define MAX_LENGTH 2048
#define LOGIC_WORE_THREAD 2
#define MAX_RECVQUE  2000000
#define MAX_SENDQUE 2000000
//4个文件工作者
#define FILE_WORKER_COUNT 4
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
    UidInvalid = 1011,
    TokenInvalid = 1012,
    CREATE_CHAT_FAILED,
    LOAD_CHAT_FAILED,
    FileNotExists,
    FileSaveRedisFailed,
    FileWritePermissionFailed,
};

enum MSG_IDS {
    ID_UPLOAD_HEAD_ICON_REQ = 1031,
    ID_UPLOAD_HEAD_ICON_RSP = 1032,
    ID_DOWN_LOAD_FILE_REQ = 1033,
    ID_DOWN_LOAD_FILE_RSP = 1034,
    ID_IMG_CHAT_UPLOAD_REQ = 1037,
    ID_IMG_CHAT_UPLOAD_RSP = 1038,
    ID_FILE_CHAT_UPLOAD_REQ = 1042,
    ID_FILE_CHAT_UPLOAD_RSP = 1043,
};

class ConfigMgr;
extern ConfigMgr gCfgMgr;

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"

#endif //RESOURCESERVER_CONST_H