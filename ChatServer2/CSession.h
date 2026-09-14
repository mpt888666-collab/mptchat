//
// Created by mpt on 2026/7/8.
//

#ifndef CHATSERVER_CSESSION_H
#define CHATSERVER_CSESSION_H
#include "const.h"
#include <boost/uuid.hpp>
#include "MsgNode.h"
class CServer;
class CSession : public std::enable_shared_from_this<CSession>{
public:
    CSession(boost::asio::io_context& io, CServer* server);

    ~CSession();

    void Start();

    void AsyncReadHead(int total_len);

    void Close();

    void AsyncReadBody(int length);

    void NotifyOffline(int uid);

    bool IsHeartbeatExpired(time_t now);

    void UpdateHeartbeat();

    boost::asio::ip::tcp::socket &GetSocket() {
        return _socket;
    }

    std::string GetSessionId() {
        return _session_id;
    }

    void SetUserId(int uid) {
        _user_uid = uid;
    }

    void Send(std::string, short);

    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession>);

    void DealExceptionSession();

private:
    boost::asio::ip::tcp::socket _socket;

    char _data[MAX_LENGTH];

    std::string _session_id;

    CServer* _server;

    std::shared_ptr<MsgNode>_recv_head_node;

    std::shared_ptr<RecvNode> _recv_msg_node;

    std::queue<std::shared_ptr<SendNode>> _send_queue;

    std::mutex _send_mutex;

    std::mutex _session_mutex;

    bool _b_close;

    bool _b_head_parse;

    int _user_uid;

    time_t _last_heartbeat;

    void asyncReadFull(std::size_t maxLength, std::function<void(const boost::system::error_code, std::size_t)> handler);

    void asyncReadLen(std::size_t read_len, std::size_t total_en, std::function<void(const boost::system::error_code, std::size_t)> handler);
};


class LogicNode {
    friend class LogicSystem;
public:
    LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recv_node);
private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};

#endif //CHATSERVER_CSESSION_H