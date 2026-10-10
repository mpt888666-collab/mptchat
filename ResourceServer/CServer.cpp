//
// Created by mpt on 2026/8/23.
//

#include "CServer.h"

#include "AsioIOServicePool.h"
#include "CSession.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
CServer::CServer(boost::asio::io_context& io_context, short port) :_io_context(io_context), _port(port),
                                                                   _acceptor(_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {

    std::cout << "Server start success, listen on port : " << _port << std::endl;
    StartAccept();
}

CServer::~CServer() {
    std::cout << "Server destruct listen on port : " << _port << std::endl;
}

void CServer::ClearSession(std::string uuid) {
    std::lock_guard<std::mutex> lock(_mtx);
    auto it = _sessions.find(uuid);
    if (it != _sessions.end()) {
        _sessions.erase(it);
    }
}

void CServer::StartAccept() {
    auto &ioc = AsioIOServicePool::GetInstance()->GetIOService();
    auto new_session = std::make_shared<CSession>(ioc, this);
    _acceptor.async_accept(new_session->GetSocket(), [this, new_session](boost::system::error_code ec) {
       if (ec) {
           std::cout << "session accept failed, error is " << ec.what() << std::endl;
       }else {
           new_session->Start();
           std::lock_guard<std::mutex> lock(_mtx);
           _sessions.insert(make_pair(new_session->GetSessionId(), new_session));
       }
        StartAccept();
    });
}


