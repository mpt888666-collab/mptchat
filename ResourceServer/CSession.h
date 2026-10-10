//
// Created by mpt on 2026/8/23.
//

#ifndef RESOURCESERVER_CSESSION_H
#define RESOURCESERVER_CSESSION_H
#include <memory>
#include <utility>
#include "const.h"
#include "MsgNode.h"
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
class CServer;
class LogicSystem;
class CSession : public std::enable_shared_from_this<CSession>{
    friend class LogicSystem;
public:
    CSession(boost::asio::io_context&, CServer* server);

    boost::asio::ip::tcp::socket& GetSocket() {
        return _socket;
    }

    void Start();
    std::string GetSessionId() {
        return _session_id;
    }

    void AsyncReadHead(int len);

    void Close();

    void Send(std::string msg, short msg_id);

    void HandleWrite(const boost::system::error_code &error, std::shared_ptr<CSession> session);

    void AsyncReadBody(int len);
private:
    boost::asio::ip::tcp::socket _socket;
    char _data[MAX_LENGTH];
    CServer* _server;
    bool _b_close;

    bool _b_head_parse;

    int _user_uid;

    std::string _session_id;

    std::shared_ptr<MsgNode>_recv_head_node;

    std::shared_ptr<RecvNode> _recv_msg_node;

    std::queue<std::shared_ptr<SendNode>> _send_queue;

    time_t _last_heartbeat;

    std::mutex _send_mutex;

    std::mutex _session_mutex;

    void asyncReadFull(std::size_t maxLength,std::function<void(const boost::system::error_code, std::size_t)> handler);

    void asyncReadLen(std::size_t read_len, std::size_t total_len,std::function<void(const boost::system::error_code, std::size_t)> handler);
};
class LogicWork;
class LogicNode {
    friend LogicWork;
public:
    LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> msg) : _session(std::move(session)), _recvnode(std::move(msg)){};
private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};

#endif //RESOURCESERVER_CSESSION_H