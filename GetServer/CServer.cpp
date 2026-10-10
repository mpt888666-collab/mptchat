//
// Created by mpt on 2026/6/29.
//

#include "CServer.h"

#include <iostream>
#include <exception>
#include <memory>

CServer::CServer(boost::asio::io_context &ioc, const unsigned short &port) : _ioc(ioc), _socket(ioc),
                                                                             _acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)){

}

void CServer::start() {
    auto self = shared_from_this();
    std::cout << "Starting server..." << std::endl;
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);
    _acceptor.async_accept(new_con->GetSocket(), [self, new_con](boost::beast::error_code ec) {
        try {
            if (ec) {
                self->start();
                return;
            }
            new_con->Start();
            self->start();
            std::cout << "connection established!" << std::endl;
        }catch (std::exception &e) {
            std::cout << "CServer start exception is: " << e.what() << std::endl;
            self->start();
        }
    });
}