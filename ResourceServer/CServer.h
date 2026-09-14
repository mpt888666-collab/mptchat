//
// Created by mpt on 2026/8/23.
//

#ifndef RESOURCESERVER_CSERVER_H
#define RESOURCESERVER_CSERVER_H
#include "const.h"
class CSession;
class CServer : public std::enable_shared_from_this<CServer>{
public:
    CServer(boost::asio::io_context &ioc, short port);

    ~CServer();

    void ClearSession(std::string uuid);

private:
    boost::asio::io_context& _io_context;
    short _port;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mtx;

    void StartAccept();
};


#endif //RESOURCESERVER_CSERVER_H
