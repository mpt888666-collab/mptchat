//
// Created by mpt on 2026/7/6.
//

#ifndef STATUSSERVER_STATUSSERVICEIMPL_H
#define STATUSSERVER_STATUSSERVICEIMPL_H
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
struct ChatServer {
    std::string host;
    std::string port;
    std::string name;
    int con_count{0};
};

class StatusServiceImpl final : public message::StatusService::Service{
public:
    StatusServiceImpl();
    grpc::Status GetChatServer(grpc::ServerContext* context, const message::GetChatServerReq* request,
        message::GetChatServerRsp* reply) override;

    grpc::Status Login(grpc::ServerContext* context, const message::LoginReq* request,
        message::LoginRsp* reply) override;

    void insertToken(int, std::string);

    ChatServer getChatServer();

private:
    std::unordered_map<std::string, ChatServer> _servers;
    int _server_index;

    std::map<int, std::string> _userMap;
    std::mutex _server_mtx;
};


#endif //STATUSSERVER_STATUSSERVICEIMPL_H