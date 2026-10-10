//
// Created by mpt on 2026/7/26.
//

#ifndef CHATSERVER_CHATGRPCCLIENT_H
#define CHATSERVER_CHATGRPCCLIENT_H
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <grpcpp/channel.h>
#include "Singleton.h"
#include <queue>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
class ChatConPool {
public:
    ChatConPool(ChatConPool&) = delete;
    ChatConPool& operator=(ChatConPool&) = delete;

    explicit ChatConPool(std::string  host, std::string  port, int size = 2);

    std::unique_ptr<message::ChatService::Stub> getConnection();

    void returnConnection(std::unique_ptr<message::ChatService::Stub> connection);

    void Close();

    ~ChatConPool();
private:
    std::queue<std::unique_ptr<message::ChatService::Stub>> _connections;
    int _pool_size;
    bool _stop;
    std::mutex _mtx;
    std::condition_variable _cv;
    std::string _host;
    std::string _port;
};

class ChatGrpcClient : public Singleton<ChatGrpcClient>{
    friend class Singleton<ChatGrpcClient>;

public:
    message::AddFriendRsp NotifyAddFriend(std::string server_ip, const message::AddFriendReq& req);

    message::AuthFriendRsp NotifyAuthFriend(std::string server_ip, const message::AuthFriendReq& req);
    message::TextChatMsgRsp NotifyTextChatMsg(std::string server_ip, const message::TextChatMsgReq& req);
    message::ImageChatMsgRsp NotifyImageChatMsg(std::string server_ip, const message::ImageChatMsgReq& req);
    message::KickUserRsp NotifyKickUser(std::string server_ip, const message::KickUserReq& req);
    message::AddGroupChatRsp NotifyAddGroupChat(std::string server_ip, const message::AddGroupChatReq &req);
private:
    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};


#endif //CHATSERVER_CHATGRPCCLIENT_H
