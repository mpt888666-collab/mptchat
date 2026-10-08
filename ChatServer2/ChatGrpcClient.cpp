//
// Created by mpt on 2026/7/26.
//

#include "ChatGrpcClient.h"
#include "ConfigMgr.h"
ChatConPool::ChatConPool(std::string  host, std::string  port, const int size) : _pool_size(size),
_stop(false), _host(std::move(host)), _port(std::move(port)){
    for (int i = 0; i < _pool_size; i++) {
        std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(_host + ":" + _port,
            grpc::InsecureChannelCredentials());
        _connections.push(message::ChatService::NewStub(channel));
    }
}

std::unique_ptr<message::ChatService::Stub> ChatConPool::getConnection() {
    std::unique_lock<std::mutex> lock(_mtx);
    _cv.wait(lock, [this]() {
        return !_connections.empty() || _stop;
    });
    if (_stop) return nullptr;
    auto res = std::move(_connections.front());
    _connections.pop();
    return res;
}

void ChatConPool::returnConnection(std::unique_ptr<message::ChatService::Stub> connection) {
    std::unique_lock<std::mutex> lock(_mtx);
    if (_stop) {
        return;
    }
    _connections.push(std::move(connection));
    _cv.notify_one();
}

void ChatConPool::Close() {
    _stop = true;
    _cv.notify_all();
}

ChatConPool::~ChatConPool() {
    std::unique_lock<std::mutex> lock(_mtx);
    Close();
    while (!_connections.empty()) {
        _connections.pop();
    }
}

ChatGrpcClient::ChatGrpcClient() {
    auto &cfg = ConfigMgr::Inst();
    std::vector<std::string> words;
    auto server_list = cfg["PeerServer"]["Servers"];
    std::stringstream ss(server_list);
    std::string word;
    while (std::getline(ss, word, ',')) {
        words.push_back(std::move(word));
    }
    for (const auto& word : words) {
        if (cfg[word]["Name"].empty()) continue;
        _pools[word] = std::make_unique<ChatConPool>(cfg[word]["Host"], cfg[word]["RPCPort"], 5);
    }
}
message::AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_ip, const message::AddFriendReq &req) {
    grpc::ClientContext context;
    message::AddFriendRsp reply;
    auto find_it = _pools.find(server_ip);
    if (find_it == _pools.end()) {
        reply.set_error(ErrorCodes::RPCFailed);
        reply.set_applyuid(req.applyuid());
        reply.set_touid(req.touid());
        return reply;
    }
    auto &pool = find_it->second;
    auto stub = pool->getConnection();

    grpc::Status status = stub->NotifyAddFriend(&context, req, &reply);
    if (status.ok()) {
        reply.set_error(ErrorCodes::Success);
        reply.set_applyuid(req.applyuid());
        reply.set_touid(req.touid());
    }else {
        reply.set_error(ErrorCodes::RPCFailed);
        reply.set_applyuid(req.applyuid());
        reply.set_touid(req.touid());
    }
    pool->returnConnection(std::move(stub));
    return reply;
}

message::AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_ip, const message::AuthFriendReq &req) {
    message::AuthFriendRsp reply;
    reply.set_error(ErrorCodes::Success);
    grpc::ClientContext context;

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        reply.set_fromuid(req.fromuid());
        reply.set_touid(req.touid());
        return reply;
    }

    auto& pool = find_iter->second;
    auto stub = pool->getConnection();
    grpc::Status status = stub->NotifyAuthFriend(&context, req, &reply);
    if (!status.ok()) {
        reply.set_error(ErrorCodes::RPCFailed);
        reply.set_fromuid(req.fromuid());
        reply.set_touid(req.touid());
        return reply;
    }
    pool->returnConnection(std::move(stub));
    return reply;
}

message::TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_ip, const message::TextChatMsgReq& req) {
    message::TextChatMsgRsp rsp;
    grpc::ClientContext context;
    rsp.set_error(ErrorCodes::Success);

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        rsp.set_thread_id(req.thread_id());
        rsp.set_error(ErrorCodes::RPCFailed);
        for (const auto &text_data : req.textmsgs()) {
            auto *new_msg = rsp.add_textmsgs();
            new_msg->set_msg_id(text_data.msg_id());
            new_msg->set_msgcontent(text_data.msgcontent());
            new_msg->set_chat_time(text_data.chat_time());
            new_msg->set_unique_id(text_data.unique_id());
        }
        return rsp;
    }
    auto &pool = find_iter->second;
    auto stub = pool->getConnection();
    grpc::Status status = stub->NotifyTextChatMsg(&context, req, &rsp);

    rsp.set_fromuid(req.fromuid());
    rsp.set_touid(req.touid());
    rsp.set_thread_id(req.thread_id());
    rsp.set_error(ErrorCodes::Success);
    for (const auto &text_data : req.textmsgs()) {
        auto *new_msg = rsp.add_textmsgs();
        new_msg->set_msg_id(text_data.msg_id());
        new_msg->set_msgcontent(text_data.msgcontent());
        new_msg->set_chat_time(text_data.chat_time());
        new_msg->set_unique_id(text_data.unique_id());
    }
    pool->returnConnection(std::move(stub));
    return rsp;
}

message::ImageChatMsgRsp ChatGrpcClient::NotifyImageChatMsg(std::string server_ip, const message::ImageChatMsgReq& req) {
    message::ImageChatMsgRsp rsp;
    grpc::ClientContext context;
    rsp.set_error(ErrorCodes::Success);
    rsp.set_fromuid(req.fromuid());
    rsp.set_touid(req.touid());
    rsp.set_thread_id(req.thread_id());
    rsp.set_message_id(req.message_id());

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    auto &pool = find_iter->second;
    auto stub = pool->getConnection();
    grpc::Status status = stub->NotifyImageChatMsg(&context, req, &rsp);
    if (!status.ok()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        rsp.set_fromuid(req.fromuid());
        rsp.set_touid(req.touid());
        rsp.set_thread_id(req.thread_id());
        rsp.set_message_id(req.message_id());
        rsp.set_total_size(req.total_size());
        pool->returnConnection(std::move(stub));
        return rsp;
    }

    rsp.set_error(ErrorCodes::Success);
    rsp.set_fromuid(req.fromuid());
    rsp.set_touid(req.touid());
    rsp.set_thread_id(req.thread_id());
    rsp.set_message_id(req.message_id());
    rsp.set_total_size(req.total_size());
    pool->returnConnection(std::move(stub));
    return rsp;
}

message::KickUserRsp ChatGrpcClient::NotifyKickUser(std::string server_ip, const message::KickUserReq &req) {
    message::KickUserRsp rsp;
    grpc::ClientContext context;
    rsp.set_error(ErrorCodes::Success);

    auto find_iter = _pools.find(server_ip);
    if (find_iter == _pools.end()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        rsp.set_uid(req.uid());
        return rsp;
    }
    auto &pool = find_iter->second;
    auto stub = pool->getConnection();
    grpc::Status status = stub->NotifyKickUser(&context, req, &rsp);
    if (!status.ok()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        rsp.set_uid(req.uid());
        pool->returnConnection(std::move(stub));
        return rsp;
    }
    rsp.set_error(ErrorCodes::Success);
    rsp.set_uid(req.uid());
    pool->returnConnection(std::move(stub));
    return rsp;
}

message::AddGroupChatRsp ChatGrpcClient::NotifyAddGroupChat(std::string server_ip, const message::AddGroupChatReq &req) {
    message::AddGroupChatRsp rsp;
    grpc::ClientContext context;

    rsp.set_error(ErrorCodes::Success);
    rsp.set_host_uid(req.host_uid());
    rsp.set_thread_id(req.thread_id());
    rsp.set_member_size(req.member_size());
    rsp.set_uid(req.uid());
    for (const auto& uids : req.members_uid()) {
        auto *membersUid = rsp.add_members_uid();
        membersUid->set_uid(uids.uid());
    }

    auto iter = _pools.find(server_ip);
    if (iter == _pools.end()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    auto& pool = iter->second;
    auto stub = pool->getConnection();
    grpc::Status status = stub->NotifyAddGroupChat(&context, req, &rsp);

    if (!status.ok()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        pool->returnConnection(std::move(stub));
        return rsp;
    }
    return rsp;
}






