//
// Created by mpt on 2026/7/26.
//

#ifndef CHATSERVER_CHATSERVICEIMPL_H
#define CHATSERVER_CHATSERVICEIMPL_H
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <grpcpp/channel.h>

#include "data.h"
#include <memory>
#include <string>
class UserMgr;
class CServer;
class ChatServiceImpl final : public message::ChatService::Service{
public:
    ChatServiceImpl();

    grpc::Status NotifyAddFriend(grpc::ServerContext *context, const message::AddFriendReq *request,
        message::AddFriendRsp *response) override;

    grpc::Status NotifyAuthFriend(grpc::ServerContext *context, const message::AuthFriendReq *request, message::AuthFriendRsp *reply) override;

    grpc::Status NotifyTextChatMsg(grpc::ServerContext* context,
        const message::TextChatMsgReq* request, message::TextChatMsgRsp* response) override;

    grpc::Status NotifyImageChatMsg(grpc::ServerContext* context,
        const message::ImageChatMsgReq* request, message::ImageChatMsgRsp* response) override;

    grpc::Status NotifyKickUser(grpc::ServerContext* context,
        const message::KickUserReq* request, message::KickUserRsp* response) override;

    grpc::Status NotifyAddGroupChat(grpc::ServerContext* context,
        const message::AddGroupChatReq* request, message::AddGroupChatRsp* response) override;

    void RegisterServer(std::shared_ptr<CServer> pServer);

    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> &userinfo);

private:
    std::shared_ptr<CServer> _p_server;
};


#endif //CHATSERVER_CHATSERVICEIMPL_H
