//
// Created by mpt on 2026/6/29.
//

#include "HttpConnection.h"
#include <cstddef>
#include <exception>
#include <iostream>

HttpConnection::HttpConnection(boost::asio::io_context &ioc) : _socket(ioc){

}

void HttpConnection::Start() {
    auto self = shared_from_this();
    boost::beast::http::async_read(_socket, _buffer, _request, [self](boost::system::error_code ec, std::size_t bytes_transfer) {
        try {
            if (ec) {
                std::cout << "http read err is " << ec.what() << std::endl;
                    return;
            }

            boost::ignore_unused(bytes_transfer);

            self->HandleReq();
            self->CheckDeadline();

        }catch (std::exception& e) {
            std::cout << "exception is " << e.what() << std::endl;
        }
    });
}

void HttpConnection::HandleReq() {
    _response.version(_request.version());
    _response.keep_alive(false);

    if (_request.method() == boost::beast::http::verb::get) {
        bool success = LogicSystem::GetInstance()->HandleGet(_request.target(), shared_from_this());
        if (!success) {
            _response.result(boost::beast::http::status::not_found);
            _response.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return ;

        }
        _response.result(boost::beast::http::status::ok);
        _response.set(boost::beast::http::field::server, "GateServer");
        WriteResponse();
        return;
    }

    if (_request.method() == boost::beast::http::verb::post) {
        bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
        if (!success) {
            _response.result(boost::beast::http::status::not_found);
            _response.set(boost::beast::http::field::content_type, "text/plain");
            boost::beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }
        _response.result(boost::beast::http::status::ok);
        _response.set(boost::beast::http::field::server, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();

    _deadline.async_wait([self](boost::system::error_code ec) {
        try {
            if (!ec) {
                self->_socket.close(ec);
            }

        }catch (std::exception& e) {
            std::cout << "HttpConnection::CheckDeadline err is " << e.what() << std::endl;
        }
    });
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    _response.content_length(_response.body().size());

    boost::beast::http::async_write(_socket, _response,[self](boost::system::error_code ec, std::size_t bytes_transfer) {
        self->_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        self->_deadline.cancel();
    });
}

boost::asio::ip::tcp::socket &HttpConnection::GetSocket() {
    return _socket;
}