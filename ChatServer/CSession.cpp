//
// Created by mpt on 2026/7/8.
//

#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include "AsioIOServicePool.h"
#include "RedisMgr.h"

CSession::CSession(boost::asio::io_context &io, CServer* server)
    : _socket(io), _server(server), _b_close(false), _b_head_parse(false), _user_uid(0)
{
    std::string uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    _session_id = uuid;
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
    _last_heartbeat = std::time(nullptr);
}

CSession::~CSession() {
    std::cout << "~CSession destruct" << std::endl;
}

void CSession::Start() {
    AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::AsyncReadHead(int total_len) {
    auto self = shared_from_this();
    asyncReadFull(HEAD_TOTAL_LEN, [self](boost::system::error_code ec, std::size_t bytes_transfered) {
        try {
            if (ec) {
                std::cout << "handle read failed, error is " << ec.what() << std::endl;
                self->Close();
                self->DealExceptionSession();
                return;
            }

            if (bytes_transfered < HEAD_TOTAL_LEN) {
                std::cout << "read length not match, read [" << bytes_transfered << "] , total ["
                    << HEAD_TOTAL_LEN << "]" << std::endl;
                self->Close();
                self->_server->ClearSession(self->_session_id);
                return;
            }

            // if (!self->_server->CheckValid(self->_session_id)) {
            //     self->Close();
            //     return;
            // }

            self->_recv_head_node->Clear();
            memcpy(self->_recv_head_node->_data, self->_data, bytes_transfered);

            short msg_id = 0;
            short msg_len = 0;
            memcpy(&msg_id, self->_recv_head_node->_data, HEAD_ID_LEN);
            memcpy(&msg_len, self->_recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_id = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
            msg_len = boost::asio::detail::socket_ops::host_to_network_short(msg_len);

            if (msg_id > MAX_LENGTH) {
                std::cout << "invalid msg_id is " << msg_id << std::endl;
                self->Close();
                self->_server->ClearSession(self->_session_id);
                return;
            }
            std::cout << "msg_id = " << msg_id << std::endl;

            if (msg_len > MAX_LENGTH) {
                std::cout << "invalid data length is " << msg_len << std::endl;
                self->Close();
                self->_server->ClearSession(self->_session_id);
                return;
            }
            std::cout << "msg_len = " << msg_len << std::endl;
            self->_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
            self->AsyncReadBody(msg_len);
        }catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    });
}

void CSession::Close() {
    {
        std::lock_guard<std::mutex> lock(_send_mutex);
        while (!_send_queue.empty()) {
            _send_queue.pop();
        }
    }
    {
        std::lock_guard<std::mutex> lock(_session_mutex);
        if (_b_close) return;
        _b_close = true;
    }
    boost::system::error_code ec;
    _socket.close(ec);
    if (_user_uid != 0) {
        auto server_name = ConfigMgr::Inst()["SelfServer"]["Name"];
        RedisMgr::GetInstance()->Del(USERIPPREFIX + std::to_string(_user_uid));
        RedisMgr::GetInstance()->HDecr(LOGIN_COUNT, server_name);
        _user_uid = 0; // ��ֹ�ظ���
    }
}

void CSession::AsyncReadBody(int total_len) {
    auto self = shared_from_this();
    asyncReadFull(total_len, [self, total_len](boost::system::error_code ec, std::size_t bytes_transfered) {
        try {
            if (ec) {
                std::cout << "handle read failed, error is " << ec.what() << std::endl;
                self->Close();
                self->DealExceptionSession();
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

            self->UpdateHeartbeat();
            LogicSystem::GetInstance()->PostMsgToQue(
                std::make_shared<LogicNode>(self, self->_recv_msg_node));

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

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recv_node)
    : _session(session), _recvnode(recv_node) {}

void CSession::Send(std::string msg, short msg_id) {
    auto self = shared_from_this();
    {
        std::lock_guard<std::mutex> lock(_send_mutex);
        if (_b_close) return;

        int size = static_cast<int>(_send_queue.size());
        if (size >= MAX_SENDQUE) {
            std::cout << "queue full" << std::endl;
            return;
        }
        _send_queue.push(std::make_shared<SendNode>(msg.c_str(), static_cast<uint16_t>(msg.size()), msg_id));

        if (_send_queue.size() > 1) return;
    }

    std::shared_ptr<SendNode> msg_node;
    {
        std::lock_guard<std::mutex> lock(_send_mutex);
        if (_send_queue.empty()) return;
        msg_node = _send_queue.front();
    }

    boost::asio::async_write(_socket,
        boost::asio::buffer(msg_node->_data, msg_node->_total_len),
        [self](const boost::system::error_code& ec, std::size_t) {
            self->HandleWrite(ec, self);
        });
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> session) {
    if (error) {
        std::cout << "handle write failed, error is " << error.what() << std::endl;
        Close();
        DealExceptionSession();
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
                [self](const boost::system::error_code& ec, std::size_t) {
                    self->HandleWrite(ec, self);
                });
        }
    }
}

void CSession::NotifyOffline(int uid) {
    json root;
    root["uid"] = uid;
    root["error"] = ErrorCodes::Success;
    Send(root.dump(), ID_NOTIFY_OFF_LINE_REQ);
}

bool CSession::IsHeartbeatExpired(time_t now) {
    auto diff_sec = std::difftime(now, _last_heartbeat);
    if (diff_sec > 60) {
        std::cout << "heartbeat expired, session id is  " << _session_id << std::endl;
        return true;
    }
    return false;
}
void CSession::UpdateHeartbeat()
{
    time_t now = std::time(nullptr);
    _last_heartbeat = now;
}

void CSession::DealExceptionSession() {
    auto self = shared_from_this();

    if (_user_uid == 0) return;

    auto uid_str = std::to_string(_user_uid);
    auto lock_key = LOCK_PREFIX + uid_str;
    auto lock_value = lock_key + "value";
    bool b_lock = RedisMgr::GetInstance()->TryLock(lock_key, lock_value, 30);
    if (!b_lock) return;

    std::string redis_session_id = "";
    auto bsuccess = RedisMgr::GetInstance()->Get(USER_SESSION_PREFIX + uid_str, redis_session_id);
    if (!bsuccess) {
        RedisMgr::GetInstance()->Unlock(lock_key, lock_value);
        return;
    }

    if (redis_session_id != _session_id) {
        _server->ClearSession(_session_id);
        RedisMgr::GetInstance()->Unlock(lock_key, lock_value);
        return;
    }
    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);

    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
    _server->ClearSession(_session_id);
    RedisMgr::GetInstance()->Unlock(lock_key, lock_value);
}
