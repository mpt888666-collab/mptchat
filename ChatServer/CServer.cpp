//
// Created by mpt on 2026/7/8.
//

#include "CServer.h"

#include "AsioIOServicePool.h"
#include "CSession.h"
#include "RedisMgr.h"
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

CServer::CServer(boost::asio::io_context& io_context, short port) :_io_context(io_context), _port(port),
                                                                   _acceptor(_io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port)), _timer(io_context, std::chrono::seconds(60)) {
    _timer.async_wait([this](boost::system::error_code ec) {
        on_timer(ec);
    });
    std::cout << "Server start success, listen on port : " << _port << std::endl;
    StartAccept();
}

void CServer::StartAccept() {
    auto& ioc = AsioIOServicePool::GetInstance()->GetIOService();

    auto new_session = std::make_shared<CSession>(ioc, this);
    _acceptor.async_accept(new_session->GetSocket(), [this, new_session](const boost::system::error_code& ec) {
        if (!ec) {
            new_session->Start();
            std::lock_guard<std::mutex> lock(_mutex);
            _sessions.insert(make_pair(new_session->GetSessionId(), new_session));
        }else {
            std::cout << "session accept failed, error is " << ec.what() << std::endl;
        }
        StartAccept();
    });
}

CServer::~CServer() {
    std::cout << "Server destruct listen on port : " << _port << std::endl;
}

void CServer::ClearSession(std::string uuid) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(uuid);
    if (it != _sessions.end()) {
        _sessions.erase(it);
    }
}

void CServer::on_timer(const boost::system::error_code &ec) {
    if (ec) {
        std::cout << "timer error: " << ec.message() << std::endl;
        return;
    }

    std::vector<std::shared_ptr<CSession>> expired_sessions;
    int session_count = 0;
    time_t now = std::time(nullptr);
    for (auto it = _sessions.begin(); it != _sessions.end(); it++) {
        bool b_expired = it->second->IsHeartbeatExpired(now);
        if (b_expired) {
            it->second->Close();
            expired_sessions.push_back(it->second);
        }else {
            session_count++;
        }
    }
    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, std::to_string(session_count));

    for (auto & session : expired_sessions) {
        session->DealExceptionSession();
        // 无论 DealExceptionSession 是否成功，都必须从 _sessions 里移除
        ClearSession(session->GetSessionId());
    }
    _timer.expires_after(std::chrono::seconds(60));
    _timer.async_wait([this](boost::system::error_code ec) {
        on_timer(ec);
    });
}

bool CServer::CheckValid(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(uuid);
    if (it != _sessions.end()) {
        return true;
    }
    return false;
}