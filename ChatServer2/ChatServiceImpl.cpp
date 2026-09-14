//
// Created by mpt on 2026/7/26.
//

#include "ChatServiceImpl.h"
#include "CServer.h"
#include "UserMgr.h"
#include "Singleton.h"
#include <nlohmann/json.hpp>
#include "CSession.h"
#include "const.h"
#include "data.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "CServer.h"
using json = nlohmann::json;
ChatServiceImpl::ChatServiceImpl() {

}

grpc::Status ChatServiceImpl::NotifyAddFriend(grpc::ServerContext *context, const message::AddFriendReq *request,
    message::AddFriendRsp *reply) {
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->getSession(touid);
    if (!session) {
        reply->set_error(ErrorCodes::Success);
        reply->set_applyuid(request->applyuid());
        reply->set_touid(request->touid());
        return grpc::Status::OK;
    }
    json rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["applyuid"] = request->applyuid();
    rtvalue["name"] = request->name();
    rtvalue["desc"] = request->desc();
    rtvalue["icon"] = request->icon();
    rtvalue["sex"] = request->sex();
    rtvalue["nick"] = request->nick();

    std::string str_json = rtvalue.dump();

    reply->set_error(ErrorCodes::Success);
    reply->set_applyuid(request->applyuid());
    reply->set_touid(request->touid());

    session->Send(str_json, ID_NOTIFY_ADD_FRIEND_REQ);

    return grpc::Status::OK;
}

grpc::Status ChatServiceImpl::NotifyAuthFriend(grpc::ServerContext *context, const message::AuthFriendReq *request, message::AuthFriendRsp *reply) {
    auto touid = request->touid();
    auto fromuid = request->fromuid();
    auto session = UserMgr::GetInstance()->getSession(touid);

    //用户不在内存中则直接返回
    if (session == nullptr) {
        reply->set_error(ErrorCodes::Success);
        reply->set_fromuid(request->fromuid());
        reply->set_touid(request->touid());
        return grpc::Status::OK;
    }

    //在内存中则直接发送通知对方
    json rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["fromuid"] = request->fromuid();
    rtvalue["touid"] = request->touid();

    std::string base_key = USER_BASE_INFO + std::to_string(fromuid);
    auto user_info = std::make_shared<UserInfo>();
    bool b_info = GetBaseInfo(base_key, fromuid, user_info);
    if (b_info) {
        rtvalue["name"] = user_info->username;
        rtvalue["nick"] = user_info->nick;
        rtvalue["icon"] = user_info->icon;
        rtvalue["sex"] = user_info->sex;
    }
    else {
        rtvalue["error"] = ErrorCodes::UidInvalid;
    }


    session->Send(rtvalue.dump(), ID_NOTIFY_AUTH_FRIEND_REQ);
    return grpc::Status::OK;
}

grpc::Status ChatServiceImpl::NotifyTextChatMsg(grpc::ServerContext* context,const message::TextChatMsgReq* request, message::TextChatMsgRsp* response){
    json root;
    json chat_datas = json::array();
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->getSession(touid);
    if (session == nullptr) {
        return grpc::Status::OK;
    }
    root["fromuid"] = request->fromuid();
    root["touid"] = request->touid();
    root["thread_id"] = request->thread_id();
    root["error"] = ErrorCodes::Success;
    for (auto& text_data : request->textmsgs()) {
        json element;
        element["message_id"] = text_data.msg_id();
        element["unique_id"] = text_data.unique_id();
        element["chat_time"] = text_data.chat_time();
        element["content"] = text_data.msgcontent();
        element["status"] = 2;
        chat_datas.push_back(element);
    }
    root["chat_datas"] = chat_datas;
    session->Send(root.dump(), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
    return grpc::Status::OK;
}

grpc::Status ChatServiceImpl::NotifyImageChatMsg(grpc::ServerContext* context, const message::ImageChatMsgReq* request, message::ImageChatMsgRsp* response){
    auto touid = request->touid();
    auto session = UserMgr::GetInstance()->getSession(touid);
    if (session == nullptr) {
        response->set_error(ErrorCodes::Success);
        response->set_fromuid(request->fromuid());
        response->set_touid(request->touid());
        response->set_thread_id(request->thread_id());
        response->set_message_id(request->message_id());
        response->set_total_size(request->total_size());
        return grpc::Status::OK;
    }

    json notify;
    notify["error"] = ErrorCodes::Success;
    notify["fromuid"] = request->fromuid();
    notify["touid"] = request->touid();
    notify["thread_id"] = request->thread_id();
    notify["message_id"] = request->message_id();
    notify["unique_id"] = request->unique_id();
    notify["name"] = request->name();
    notify["md5"] = request->md5();
    notify["chat_time"] = request->chat_time();
    notify["status"] = request->status();
    notify["type"] = request->type();
    notify["total_size"] = request->total_size();

    session->Send(notify.dump(), ID_NOTIFY_IMG_CHAT_MSG_REQ);

    response->set_error(ErrorCodes::Success);
    response->set_fromuid(request->fromuid());
    response->set_touid(request->touid());
    response->set_thread_id(request->thread_id());
    response->set_message_id(request->message_id());
    response->set_total_size(request->total_size());
    return grpc::Status::OK;
}

bool ChatServiceImpl::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    //优先查redis中查询用户信息
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        json root = json::parse(info_str);
        userinfo->uid = root["uid"].get<int>();
        userinfo->username = root["name"].get<std::string>();
        userinfo->password = root["pwd"].get<std::string>();
        userinfo->email = root["email"].get<std::string>();
        userinfo->nick = root["nick"].get<std::string>();
        userinfo->desc = root["desc"].get<std::string>();
        userinfo->sex = root["sex"].get<int>();
        userinfo->icon = root["icon"].get<std::string>();
        std::cout << "user login uid is  " << userinfo->uid << " name  is "
            << userinfo->username << " pwd is " << userinfo->password << " email is " << userinfo->email << std::endl;
    }
    else {
        //redis中没有则查询mysql
        //查询数据库
        std::shared_ptr<UserInfo> user_info = nullptr;
        user_info = MysqlMgr::GetInstance()->GetUserInfoById(uid);
        if (user_info == nullptr) {
            return false;
        }

        userinfo = user_info;

        //将数据库内容写入redis缓存
        json redis_root;
        redis_root["uid"] = uid;
        redis_root["pwd"] = userinfo->password;
        redis_root["name"] = userinfo->username;
        redis_root["email"] = userinfo->email;
        redis_root["nick"] = userinfo->nick;
        redis_root["desc"] = userinfo->desc;
        redis_root["sex"] = userinfo->sex;
        redis_root["icon"] = userinfo->icon;
        RedisMgr::GetInstance()->Set(base_key, redis_root.dump());
    }

    return true;
}

grpc::Status ChatServiceImpl::NotifyKickUser(grpc::ServerContext* context,const message::KickUserReq* request, message::KickUserRsp* response){
    auto uid = request->uid();
    auto session = UserMgr::GetInstance()->getSession(uid);
    if(session == nullptr){
        response->set_error(ErrorCodes::Success);
        response->set_uid(uid);
        return grpc::Status::OK;
    }
    session->NotifyOffline(uid);
    _p_server->ClearSession(session->GetSessionId());
    std::cout << "kftr" << std::endl;

    return grpc::Status::OK;
}

void ChatServiceImpl::RegisterServer(std::shared_ptr<CServer> pServer)
{
    _p_server = pServer;
}
