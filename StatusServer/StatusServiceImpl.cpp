//
// Created by mpt on 2026/7/6.
//

#include "StatusServiceImpl.h"
#include <boost/uuid.hpp>
#include "RedisMgr.h"
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
std::string generate_unique_string() {
    // 创建UUID对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();

    // 将UUID转换为字符串
    std::string unique_string = to_string(uuid);

    return unique_string;
}

grpc::Status StatusServiceImpl::GetChatServer(grpc::ServerContext* context, const message::GetChatServerReq* request, message::GetChatServerRsp* reply)
{
    //std::string prefix("mpt status server has received :  ");
    std::cout << "GetChatServer called" << std::endl;
    auto& cfg = ConfigMgr::Inst();
    const auto& server = getChatServer();

    // 返回负载均衡结果：当前在线连接数最少的 ChatServer 实例。
    // 抢锁失败或配置里没有可用的 chatserver 时 host/port 为空，交给调用方返回 RPCGetFailed。
    if (server.host.empty() || server.port.empty()) {
        std::cout << "GetChatServer: no available chatserver" << std::endl;
        reply->set_error(ErrorCodes::RPCFailed);
        return grpc::Status::OK;
    }

    reply->set_chat_host(server.host);
    reply->set_chat_port(server.port);
    reply->set_res_host(cfg["ResourceServer"]["Host"]);
    reply->set_res_port(cfg["ResourceServer"]["Port"]);
    reply->set_error(ErrorCodes::Success);
    reply->set_token(generate_unique_string());

    insertToken(request->uid(), reply->token());
    return grpc::Status::OK;
}

grpc::Status StatusServiceImpl::Login(grpc::ServerContext* context, const message::LoginReq* request,
        message::LoginRsp* reply) {
    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::cout << "token: " << token << std::endl;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    std::cout << "token_value: " << token_value << std::endl;

    if (!success) {
        reply->set_error(ErrorCodes::UidInvalid);
        return grpc::Status::OK;
    }

    if (token_value != token) {
        reply->set_error(ErrorCodes::TokenInvalid);
        return grpc::Status::OK;
    }
    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    return grpc::Status::OK;
}

StatusServiceImpl::StatusServiceImpl():_server_index(0)
{
    auto& cfg = ConfigMgr::Inst();
    std::vector<std::string> words;
    auto server_list = cfg["chatservers"]["Name"];
    std::stringstream ss(server_list);
    std::string word;
    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }
    for (const auto& word : words) {
        ChatServer server;
        server.port = cfg[word]["Port"];
        server.host = cfg[word]["Host"];
        server.name = cfg[word]["Name"];
        _servers[server.name] = server;
    }
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    RedisMgr::GetInstance()->Set(token_key, token);
}

ChatServer StatusServiceImpl::getChatServer() {
    std::lock_guard<std::mutex> guard(_server_mtx);
    auto minServer = _servers.begin()->second;
    auto lock_key = LOCK_PREFIX;
    auto lock_value = LOCK_PREFIX;
    bool b_lock = RedisMgr::GetInstance()->TryLock(lock_key, lock_value, 30);
    if (b_lock) {
        auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, minServer.name);
        if (count_str.empty()) {
            //不存在则默认设置为最大
            minServer.con_count = 0;
        }
        else {
            minServer.con_count = std::stoi(count_str);
        }


        // 使用范围基于for循环
        for (auto& server : _servers) {

            if (server.second.name == minServer.name) {
                continue;
            }

            auto count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server.second.name);
            if (count_str.empty()) {
                server.second.con_count = 0;
            }
            else {
                server.second.con_count = std::stoi(count_str);
            }

            if (server.second.con_count < minServer.con_count) {
                minServer = server.second;
            }
        }
        RedisMgr::GetInstance()->Unlock(lock_key, lock_value);

        return minServer;
    }
    RedisMgr::GetInstance()->Unlock(lock_key, lock_value);
    return ChatServer();
}