//
// Created by mpt on 2026/7/8.
//

#ifndef CHATSERVER_CSERVER_H
#define CHATSERVER_CSERVER_H
#include "const.h"
#include <map>
#include <memory>
#include <mutex>
#include <string>
class CSession;
class CServer {
public:
    CServer(boost::asio::io_context& io_context, short port);
    ~CServer();
    void ClearSession(std::string);

    void on_timer(const boost::system::error_code &ec);

    bool CheckValid(std::string uuid);

private:
    void StartAccept();

    boost::asio::io_context &_io_context;
    short _port;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
    boost::asio::steady_timer _timer;
};


#endif //CHATSERVER_CSERVER_H