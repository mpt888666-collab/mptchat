//
// Created by mpt on 2026/8/23.
//

#include "CSession.h"
#include <boost/uuid.hpp>

#include "CServer.h"
#include "LogicSystem.h"
#include <functional>
#include <boost/asio/post.hpp>
#include <cstddef>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
CSession::CSession(boost::asio::io_context &io, CServer* server)
    : _socket(io), _server(server), _b_close(false), _b_head_parse(false), _user_uid(0)
{
    std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    _session_id = uuid;
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
    _last_heartbeat = std::time(nullptr);
}

void CSession::Start() {
    AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::AsyncReadHead(int total_len) {
    auto self = shared_from_this();
    asyncReadFull(total_len, [this, self](const boost::system::error_code &ec, std::size_t bytes) {
        if (ec) {
            std::cout << "handle read failed, error is " << ec.what() << std::endl;
            self->Close();
            self->_server->ClearSession(_session_id);
            return;
        }

        if (bytes < HEAD_TOTAL_LEN) {
            std::cout << "read length not match, read [" << bytes << "] , total ["
                    << HEAD_TOTAL_LEN << "]" << std::endl;
            self->Close();
            self->_server->ClearSession(self->_session_id);
            return;
        }
        uint16_t net_id;
        uint32_t net_len;
        memcpy(&net_id, _data, HEAD_ID_LEN);
        memcpy(&net_len, _data + HEAD_ID_LEN, HEAD_DATA_LEN);
        auto host_id = boost::asio::detail::socket_ops::network_to_host_short(net_id);
        auto host_len = boost::asio::detail::socket_ops::network_to_host_long(net_len);
        memcpy(_recv_head_node->_data, &host_id, HEAD_ID_LEN);
        memcpy(_recv_head_node->_data + HEAD_ID_LEN, &host_len, HEAD_DATA_LEN);
        std::cout << "msg_id = " << host_id << std::endl;

        if (host_len > MAX_LENGTH) {
            std::cout << "invalid data length is " << host_len << std::endl;
            self->Close();
            self->_server->ClearSession(self->_session_id);
            return;
        }
        std::cout << "msg_len = " << host_len << std::endl;
        self->_recv_msg_node = std::make_shared<RecvNode>(host_id, host_len);
        self->AsyncReadBody(host_len);
    });
}

void CSession::AsyncReadBody(int total_len) {
    auto self = shared_from_this();
    asyncReadFull(total_len, [self, total_len](boost::system::error_code ec, std::size_t bytes_transfered) {
        try {
            if (ec) {
                std::cout << "handle read failed, error is " << ec.what() << std::endl;
                self->Close();
                self->_server->ClearSession(self->_session_id);
                return;
            }

            if (bytes_transfered < total_len) {
                std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
                    << total_len << "]" << std::endl;
                self->Close();
                self->_server->ClearSession(self->_session_id);
                return;
            }

            // if (!self->_server->CheckValid(self->_session_id)) {
            //     self->Close();
            //     return;
            // }

            memcpy(self->_recv_msg_node->_data, self->_data, bytes_transfered);
            self->_recv_msg_node->_cur_len += bytes_transfered;
            self->_recv_msg_node->_data[self->_recv_msg_node->_total_len] = '\0';
            std::hash<std::string> hash;
            int index = hash(self->_session_id) % LOGIC_WORE_THREAD;
            LogicSystem::GetInstance()->PostMsgToQue(
                std::make_shared<LogicNode>(self, self->_recv_msg_node), index);

            self->AsyncReadHead(HEAD_TOTAL_LEN);
        }catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    });
}

void CSession::asyncReadFull(std::size_t maxLength,
    std::function<void(const boost::system::error_code, std::size_t)> handler)
{
    memset(_data, 0, maxLength);
    asyncReadLen(0, maxLength, handler);
}

void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len,
    std::function<void(const boost::system::error_code, std::size_t)> handler)
{
    auto self = shared_from_this();
    _socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len),
        [self, handler, read_len, total_len](const boost::system::error_code& ec, std::size_t bytesTransfered) {
            if (ec) {
                handler(ec, read_len + bytesTransfered);
                return;
            }
            if (read_len + bytesTransfered >= total_len) {
                handler(ec, read_len + bytesTransfered);
                return;
            }
            self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
        });
}

void CSession::Close() {
    {
        std::lock_guard<std::mutex> lock(_session_mutex);
        if (_b_close) return;
        _b_close = true;
    }
    boost::system::error_code ec;
    _socket.close(ec);
}

void CSession::Send(std::string msg, short msg_id) {
    auto self = shared_from_this();
    auto executor = _socket.get_executor();
    boost::asio::post(executor, [self, msg = std::move(msg), msg_id]() {
        std::lock_guard<std::mutex> lock(self->_send_mutex);
        if (self->_b_close) return;

        int size = static_cast<int>(self->_send_queue.size());
        if (size >= MAX_SENDQUE) {
            std::cout << "queue full" << std::endl;
            return;
        }
        self->_send_queue.push(std::make_shared<SendNode>(msg.c_str(), static_cast<uint16_t>(msg.size()), msg_id));

        if (self->_send_queue.size() > 1) return;

        auto msg_node = self->_send_queue.front();
        boost::asio::async_write(self->_socket,
            boost::asio::buffer(msg_node->_data, msg_node->_total_len),
            [self, msg_node](const boost::system::error_code& ec, std::size_t) {
                self->HandleWrite(ec, self);
            });
    });
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> session) {
    if (error) {
        std::cout << "handle write failed, error is " << error.what() << std::endl;
        Close();
        session->_server->ClearSession(session->_session_id);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(_send_mutex);
        if (!_send_queue.empty()) _send_queue.pop();

        if (!_send_queue.empty()) {
            auto msg_node = _send_queue.front();
            auto self = shared_from_this();
            boost::asio::async_write(_socket,
                boost::asio::buffer(msg_node->_data, msg_node->_total_len),
                [self, msg_node](const boost::system::error_code& ec, std::size_t) {
                    self->HandleWrite(ec, self);
                });
        }
    }
}
