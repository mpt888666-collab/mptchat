//
// Created by mpt on 2026/7/1.
//
#include "VarifyGrpcClient.h"
RPConPool::RPConPool(size_t poolSize, std::string host, std::string port) :_poolSize(poolSize), _host(host), _port(port) {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
    for (int i = 0; i < poolSize; ++i) {
        _connections.push(message::VarifyService::NewStub(channel));
    }
}

std::unique_ptr<message::VarifyService::Stub> RPConPool::getConnection() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock, [this]() {
        return !_connections.empty() || _b_stop;
    });
    if (_b_stop) {
        return nullptr;
    }
    auto connection = std::move(_connections.front());
    _connections.pop();
    return connection;
}

void RPConPool::returnConnection(std::unique_ptr<message::VarifyService::Stub> context) {
    std::unique_lock<std::mutex> lock(_mtx);
    if (_b_stop) {
        return;
    }
    _connections.push(std::move(context));
    lock.unlock();
    _cv.notify_one();
}

