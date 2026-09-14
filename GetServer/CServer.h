//
// Created by mpt on 2026/6/29.
//

#ifndef GETSERVER_CSERVER_H
#define GETSERVER_CSERVER_H
#include <memory>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "HttpConnection.h"
#include "AsioIOServicePool.h"
class CServer : public std::enable_shared_from_this<CServer>{
public:
    CServer(boost::asio::io_context& ioc, const unsigned short &port);

    void start();

private:
    boost::asio::io_context& _ioc;
    boost::asio::ip::tcp::socket _socket;
    boost::asio::ip::tcp::acceptor  _acceptor;

};


#endif //GETSERVER_CSERVER_H