//
// Created by mpt on 2026/6/29.
//

#ifndef GETSERVER_HTTPCONNECTION_H
#define GETSERVER_HTTPCONNECTION_H
#include <memory>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include "LogicSystem.h"
class HttpConnection : public std::enable_shared_from_this<HttpConnection>{
public:
    friend class LogicSystem;
    explicit HttpConnection(boost::asio::io_context &ioc);

    void Start();

    boost::asio::ip::tcp::socket& GetSocket();
private:
    boost::asio::ip::tcp::socket _socket;

    boost::beast::flat_buffer  _buffer{ 8192 };

    boost::beast::http::request<boost::beast::http::dynamic_body> _request;

    boost::beast::http::response<boost::beast::http::dynamic_body> _response;

    boost::asio::steady_timer _deadline{
        _socket.get_executor(), std::chrono::seconds(60) };

    void HandleReq();

    void CheckDeadline();

    void WriteResponse();
};


#endif //GETSERVER_HTTPCONNECTION_H