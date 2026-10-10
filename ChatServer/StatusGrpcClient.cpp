//
// Created by mpt on 2026/7/6.
//

#include "StatusGrpcClient.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

StatusConPool::StatusConPool(std::string host, std::string port, int num) : _poolSize(num), _host(std::move(host)), _port(std::move(port)){
    for (size_t i = 0; i < num; i++) {
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(_host + ":" + _port,
                grpc::InsecureChannelCredentials());
        _connections.push(message::StatusService::NewStub(channel));
    }
}

std::unique_ptr<message::StatusService::Stub> StatusConPool::GetConnection() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock, [this]() {
       return !_connections.empty() || _stop;
    });

    if (_stop) {
        return nullptr;
    }

    auto res = std::move(_connections.front());
    _connections.pop();
    return res;
}

void StatusConPool::ReturnConnection(std::unique_ptr<message::StatusService::Stub> connection) {
    std::unique_lock<std::mutex> lock(_mtx);
    if (_stop) {
        return;
    }
    _connections.push(std::move(connection));
    lock.unlock();
    _cv.notify_one();
}

void StatusConPool::Close() {
    _stop = true;
    _cv.notify_all();
}



StatusConPool::~StatusConPool() {
    std::lock_guard<std::mutex> lock(_mtx);
    Close();
    while (!_connections.empty()) {
        _connections.pop();
    }
}

message::GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
    std::cout << "hhere" << std::endl;
    grpc::ClientContext context;
    message::GetChatServerRsp reply;
    message::GetChatServerReq request;
    request.set_uid(uid);
    auto stub = _pool->GetConnection();

    grpc::Status status = stub->GetChatServer(&context, request, &reply);
    if (status.ok()) {
        std::cout << "StatusServer ok" << std::endl;
        _pool->ReturnConnection(std::move(stub));
        return reply;
    }
    else {
        std::cout << "grpc error code = "
              << status.error_code() << std::endl;

        std::cout << "grpc error message = "
                  << status.error_message() << std::endl;

        std::cout << "grpc error details = "
                  << status.error_details() << std::endl;
        _pool->ReturnConnection(std::move(stub));
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }
}

message::LoginRsp StatusGrpcClient::Login(int uid, std::string token) {
    grpc::ClientContext context;
    message::LoginRsp reply;
    message::LoginReq request;

    request.set_uid(uid);
    request.set_token(token);
    auto stub = _pool->GetConnection();
    auto status = stub->Login(&context, request, &reply);

    if (status.ok()) {
        std::cout << "Login ok" << std::endl;
    }else {
        std::cout << "grpc error code = "
              << status.error_code() << std::endl;

        std::cout << "grpc error message = "
                << status.error_message() << std::endl;

        std::cout << "grpc error details = "
                  << status.error_details() << std::endl;

        reply.set_error(ErrorCodes::RPCFailed);
    }
    _pool->ReturnConnection(std::move(stub));
    return reply;

}

StatusGrpcClient::StatusGrpcClient()
{
    auto& gCfgMgr = ConfigMgr::Inst();
    std::string host = gCfgMgr["StatusServer"]["Host"];
    std::string port = gCfgMgr["StatusServer"]["Port"];
    std::cout << host << ":" << port << std::endl;
    _pool.reset(new StatusConPool( host, port, 5));
}

