//
// Created by mpt on 2026/6/29.
//

#ifndef GETSERVER_LOGICSYSTEM_H
#define GETSERVER_LOGICSYSTEM_H
#include <map>
#include "VarifyGrpcClient.h"
#include "StatusGrpcClient.h"
#include "Singleton.h"
#include "HttpConnection.h"
#include <nlohmann/json.hpp>
#include "const.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
using json = nlohmann::json;

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
class LogicSystem : public Singleton<LogicSystem>{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();

    bool HandleGet(std::string url, std::shared_ptr<HttpConnection> conn);

    bool HandlePost(std::string url, std::shared_ptr<HttpConnection> conn);

    void RegGet(std::string url, HttpHandler handler);

    void RegPost(std::string url, HttpHandler handler);

private:
    LogicSystem();

    std::map<std::string, HttpHandler> _get_handlers;

    std::map<std::string, HttpHandler> _post_handlers;
};


#endif //GETSERVER_LOGICSYSTEM_H