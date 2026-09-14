//
// Created by mpt on 2026/7/8.
//

#include "LogicSystem.h"

#include "ChatGrpcClient.h"
#include "CServer.h"
#include "StatusGrpcClient.h"
#include "CSession.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include "UserMgr.h"
#include "data.h"
#include "AsioIOServicePool.h"
#include "utils.h"
LogicSystem::LogicSystem() : _b_stop(false), _p_server(nullptr){
    RegisterCallBacks();

    _worker_thread = std::thread (&LogicSystem::DealMsg, this);
}

void LogicSystem::SetServer(std::shared_ptr<CServer> pserver) {
    _p_server = pserver;
}

void LogicSystem::RegisterCallBacks() {
    _fun_callbacks[MSG_IDS::MSG_CHAT_LOGIN] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {

        json req_json;
        json root;
        req_json = json::parse(msg_data);
        auto uid = req_json["uid"].get<int>();
        auto token = req_json["token"].get<std::string>();
        root["uid"] = uid;
        root["token"] = token;
        std::cout << "user login uid is  " << req_json["uid"].get<int>() <<  " user token  is "
        << req_json["token"].get<std::string>() << std::endl;
        auto rsp = StatusGrpcClient::GetInstance()->Login(uid, token);
        if (rsp.error() != ErrorCodes::Success) {
            root["error"] = rsp.error();
            session->Send(root.dump(), MSG_IDS::MSG_CHAT_LOGIN_RSP);
            return;
        }
        std::string val_token = "";
        bool success = RedisMgr::GetInstance()->Get(USERTOKENPREFIX + std::to_string(uid), val_token);
        if (!success) {
            root["error"] = ErrorCodes::UidInvalid;
            session->Send(root.dump(), MSG_IDS::MSG_CHAT_LOGIN_RSP);
            return;
        }

        if (token != val_token) {
            root["error"] = ErrorCodes::TokenInvalid;
            session->Send(root.dump(), MSG_IDS::MSG_CHAT_LOGIN_RSP);
            return;
        }

        root["error"] = ErrorCodes::Success;
        auto uid_ip_key = USERIPPREFIX + std::to_string(uid);
        std::string uid_ip_value = "";
        bool b_ip = RedisMgr::GetInstance()->Get(uid_ip_key, uid_ip_value);
        auto lock_key = LOCK_PREFIX + std::to_string(uid);
        auto lock_key_value = lock_key + "value";
        bool b_lock = RedisMgr::GetInstance()->Lock(lock_key, lock_key_value, 30);
        if (b_ip) {
            auto& cfg = ConfigMgr::Inst();
            auto self_name = cfg["SelfServer"]["Name"];
            if (uid_ip_value == self_name) {
                auto old_session = UserMgr::GetInstance()->getSession(uid);
                if (old_session) {
                    old_session->NotifyOffline(uid);
                    //清除旧的连接
                    _p_server->ClearSession(old_session->GetSessionId());
                }
            }else {
                message::KickUserReq kick_req;
                kick_req.set_uid(uid);
                ChatGrpcClient::GetInstance()->NotifyKickUser(uid_ip_value, kick_req);
            }
            RedisMgr::GetInstance()->Unlock(lock_key, lock_key_value);
        }
        std::string base_key = USER_BASE_INFO + std::to_string(uid);
        auto user_info = std::make_shared<UserInfo>();
        bool b_base = GetBaseInfo(base_key, uid, user_info);
        if (!b_base) {
            root["error"] = ErrorCodes::UidInvalid;
            return;
        }

        root["uid"] = uid;
        root["pwd"] = user_info->password;
        root["name"] = user_info->username;
        root["email"] = user_info->email;
        root["nick"] = user_info->nick;
        root["desc"] = user_info->desc;
        root["sex"] = user_info->sex;
        root["icon"] = user_info->icon;

        std::vector<std::shared_ptr<ApplyInfo>> apply_list;
        auto b_apply = MysqlMgr::GetInstance()->GetFriendApplyInfo(uid,apply_list);
        if (b_apply) {
            for (const auto& apply : apply_list) {
                json obj;
                obj["name"] = apply->_name;
                obj["uid"] = apply->_uid;
                obj["icon"] = apply->_icon;
                obj["nick"] = apply->_nick;
                obj["sex"] = apply->_sex;
                obj["desc"] = apply->_desc;
                obj["status"] = apply->_status;
                root["apply_list"].emplace_back(obj);
            }
        }

        std::vector<std::shared_ptr<UserInfo>> friend_list;
        bool b_friend_list = MysqlMgr::GetInstance()->GetFriendList(uid, friend_list);
        for (auto& friend_ele : friend_list) {
            json obj;
            obj["name"] = friend_ele->username;
            obj["uid"] = friend_ele->uid;
            obj["icon"] = friend_ele->icon;
            obj["nick"] = friend_ele->nick;
            obj["sex"] = friend_ele->sex;
            obj["desc"] = friend_ele->desc;
            obj["back"] = friend_ele->back;
            root["friend_list"].emplace_back(obj);
        }

        auto server_name = ConfigMgr::Inst()["SelfServer"]["Name"];
        RedisMgr::GetInstance()->HIncr(LOGIN_COUNT, server_name);
                session->SetUserId(uid);

                // Kick old session if same uid already logged in
        auto old_session = UserMgr::GetInstance()->getSession(uid);
        if (old_session) {
            std::cout << "kicking old session for uid " << uid << std::endl;
            auto &ioc = AsioIOServicePool::GetInstance()->GetIOService();
            boost::asio::post(ioc, [old_session]() {
                old_session->Close();
            });
        }

        std::string  ipkey = USERIPPREFIX + std::to_string(uid);
        RedisMgr::GetInstance()->Set(ipkey, server_name);

        UserMgr::GetInstance()->setSession(uid, session);
        session->Send(root.dump(), MSG_CHAT_LOGIN_RSP);
    };

    _fun_callbacks[ID_SEARCH_USER_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        auto uid_str = req_json["uid"].get<std::string>();
        std::cout << "user SearchInfo uid is  " << uid_str << std::endl;
        json root;
        bool b_digital = isPureDigit(uid_str);
        if (b_digital) {
            GetUserByUid(uid_str, root);
        }else {
            GetUserByName(uid_str, root);
        }
        session->Send(root.dump(), ID_SEARCH_USER_RSP);
    };

    _fun_callbacks[ID_ADD_FRIEND_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        auto uid = req_json["uid"].get<int>();
        auto applyname = req_json["applyname"].get<std::string>();
        auto touid = req_json["touid"].get<int>();
        auto bakname = req_json["bakname"].get<std::string>();
        std::cout << "user login uid is  " << uid << " applyname  is "
        << applyname << " bakname is " << bakname << " touid is " << touid << std::endl;

        json root;
        root["error"] = ErrorCodes::Success;

        //鍏堟洿鏂版暟鎹簱
        MysqlMgr::GetInstance()->AddFriendApply(uid, touid);

        auto to_str = std::to_string(touid);
        auto to_ip_key = USERIPPREFIX + to_str;
        std::string to_ip_value;
        bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
        if (!b_ip) {
            session->Send(root.dump(), ID_ADD_FRIEND_RSP);
            return;
        }
        auto& cfg = ConfigMgr::Inst();
        auto self_name = cfg["SelfServer"]["Name"];

        std::string base_key = USER_BASE_INFO + std::to_string(uid);
        auto apply_info = std::make_shared<UserInfo>();
        bool b_info = GetBaseInfo(base_key, uid, apply_info);

        if (to_ip_value == self_name) {
            auto to_session = UserMgr::GetInstance()->getSession(touid);
            if (to_session) {
                // 鍙戠粰琚敵璇蜂汉锛氶€氱煡鏈変汉瑕佸姞浣?
                json notify_json;
                notify_json["error"] = ErrorCodes::Success;
                notify_json["applyuid"] = uid;
                notify_json["name"] = applyname;
                notify_json["desc"] = "";
                if (b_info) {
                    notify_json["icon"] = apply_info->icon;
                    notify_json["sex"] = apply_info->sex;
                    notify_json["nick"] = apply_info->nick;
                }
                to_session->Send(notify_json.dump(), ID_NOTIFY_ADD_FRIEND_REQ);
            }
            // 鍙戠粰鐢宠浜猴細鐢宠宸叉彁浜?
            session->Send(root.dump(), ID_ADD_FRIEND_RSP);
            return;
        }

        message::AddFriendReq add_req;
        add_req.set_applyuid(uid);
        add_req.set_touid(touid);
        add_req.set_name(applyname);
        add_req.set_desc("");
        if (b_info) {
            add_req.set_icon(apply_info->icon);
            add_req.set_sex(apply_info->sex);
            add_req.set_nick(apply_info->nick);
        }
        ChatGrpcClient::GetInstance()->NotifyAddFriend(to_ip_value, add_req);
        session->Send(root.dump(), ID_ADD_FRIEND_RSP);
    };

    _fun_callbacks[ID_AUTH_FRIEND_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        auto uid = req_json["fromuid"].get<int>();
        auto touid = req_json["touid"].get<int>();
        auto back_name = req_json["back"].get<std::string>();
        std::cout << "from " << uid << " auth friend to " << touid << std::endl;

        json root;
        root["error"] = ErrorCodes::Success;
        auto user_info = std::make_shared<UserInfo>();
        std::string base_key = USER_BASE_INFO + std::to_string(touid);
        bool b_info = GetBaseInfo(base_key, touid, user_info);
        if (b_info) {
            root["name"] = user_info->username;
            root["nick"] = user_info->nick;
            root["icon"] = user_info->icon;
            root["sex"] = user_info->sex;
            root["uid"] = touid;
        }
        else {
            root["error"] = ErrorCodes::UidInvalid;
        }

        //鍏堟洿鏂版暟鎹簱
        MysqlMgr::GetInstance()->AuthFriendApply(uid, touid);
        //
        //鏇存柊鏁版嵁搴撴坊鍔犲ソ鍙?
        MysqlMgr::GetInstance()->AddFriend(uid, touid,back_name);
        auto to_str = std::to_string(touid);
        auto to_ip_key = USERIPPREFIX + to_str;
        std::string to_ip_value = "";
        bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
        if (!b_ip) {
            session->Send(root.dump(), ID_AUTH_FRIEND_RSP);
            return;
        }

        auto& cfg = ConfigMgr::Inst();
        auto self_name = cfg["SelfServer"]["Name"];
        if (to_ip_value == self_name) {
            auto to_session = UserMgr::GetInstance()->getSession(touid);
            if (to_session) {
                //鍦ㄥ唴瀛樹腑鍒欑洿鎺ュ彂閫侀€氱煡瀵规柟
                json  notify;
                notify["error"] = ErrorCodes::Success;
                notify["fromuid"] = uid;
                notify["touid"] = touid;
                std::string base_key = USER_BASE_INFO + std::to_string(uid);
                auto user_info = std::make_shared<UserInfo>();
                bool b_info = GetBaseInfo(base_key, uid, user_info);
                if (b_info) {
                    notify["name"] = user_info->username;
                    notify["nick"] = user_info->nick;
                    notify["icon"] = user_info->icon;
                    notify["sex"] = user_info->sex;
                }
                else {
                    notify["error"] = ErrorCodes::UidInvalid;
                }

                to_session->Send(notify.dump(), ID_NOTIFY_AUTH_FRIEND_REQ);
            }
            session->Send(root.dump(), ID_AUTH_FRIEND_RSP);
            return ;
        }

        message::AuthFriendReq auth_req;
        auth_req.set_fromuid(uid);
        auth_req.set_touid(touid);

        //鍙戦€侀€氱煡
        ChatGrpcClient::GetInstance()->NotifyAuthFriend(to_ip_value, auth_req);
        session->Send(root.dump(), ID_AUTH_FRIEND_RSP);
    };

    _fun_callbacks[ID_TEXT_CHAT_MSG_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        json root;
        auto from_uid = req_json["fromuid"].get<int>();
        auto to_uid = req_json["touid"].get<int>();
        auto thread_id = req_json.contains("thread_id") ? req_json["thread_id"].get<int>() : 0;
        if (thread_id <= 0) {
            root["error"] = ErrorCodes::Error_Json;
            session->Send(root.dump(), ID_TEXT_CHAT_MSG_RSP);
            return;
        }
        auto contents = req_json.contains("text_array") ? req_json["text_array"] : json::array();
        std::cout << "from " << from_uid << " send msg to " << to_uid << " content is " << contents << std::endl;

        root["error"] = ErrorCodes::Success;
        root["fromuid"] = from_uid;
        root["touid"] = to_uid;
        root["thread_id"] = thread_id;
        root["text_array"] = contents;
        std::vector<std::shared_ptr<ChatMessage>> chat_datas;
        auto timestamp = getCurrentTimestamp();
        for (auto &txt_obj : contents) {
            if (!txt_obj.contains("content")) {
                continue;
            }
            auto unique_id = txt_obj.contains("unique_id") ? txt_obj["unique_id"].get<std::string>() : "";
            auto content = txt_obj["content"].get<std::string>();
            auto chat_msg = std::make_shared<ChatMessage>();
            chat_msg->chat_time = timestamp;
            chat_msg->sender_id = from_uid;
            chat_msg->recv_id = to_uid;
            chat_msg->unique_id = unique_id;
            chat_msg->thread_id = thread_id;
            chat_msg->content = content;
            chat_msg->status = 2;
            chat_msg->type = MsgType::MSG_TYPE_TEXT;
            chat_datas.push_back(chat_msg);
        }

        MysqlMgr::GetInstance()->AddChatMsg(chat_datas);
        for (const auto& chat_data : chat_datas) {
            json chat_msg;
            chat_msg["message_id"] = chat_data->message_id;
            chat_msg["unique_id"] = chat_data->unique_id;
            chat_msg["content"] = chat_data->content;
            chat_msg["status"] = chat_data->status;
            chat_msg["chat_time"] = chat_data->chat_time;
            root["chat_datas"].push_back(chat_msg);
        }


        auto touid_str = std::to_string(to_uid);
        auto to_ip_key = USERIPPREFIX + touid_str;
        std::string to_ip_value = "";
        bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
        if (!b_ip) {
            session->Send(root.dump(), ID_TEXT_CHAT_MSG_RSP);
            return;
        }
        auto &cfg = ConfigMgr::Inst();
        auto self_name = cfg["SelfServer"]["Name"];
        if (to_ip_value == self_name) {
            auto to_session = UserMgr::GetInstance()->getSession(to_uid);
            if (to_session) {
                to_session->Send(root.dump(), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
            }
            session->Send(root.dump(), ID_TEXT_CHAT_MSG_RSP);
            return;
        }

        message::TextChatMsgReq text_msg_req;
        text_msg_req.set_touid(to_uid);
        text_msg_req.set_fromuid(from_uid);
        text_msg_req.set_thread_id(thread_id);

        for (const auto& chat_data : chat_datas) {
            auto *text_content = text_msg_req.add_textmsgs();
            text_content->set_unique_id(chat_data->unique_id);
            text_content->set_msg_id(chat_data->message_id);
            text_content->set_msgcontent(chat_data->content);
            text_content->set_chat_time(chat_data->chat_time);
        }

        ChatGrpcClient::GetInstance()->NotifyTextChatMsg(to_ip_value, text_msg_req);
    };
    _fun_callbacks[ID_HEART_BEAT_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        json root;
        auto uid = req_json["fromuid"].get<int>();
        std::cout << "receive heart beat msg, uid is " << uid << std::endl;
        root["error"] = ErrorCodes::Success;
        session->Send(root.dump(), ID_HEARTBEAT_RSP);
    };

    _fun_callbacks[ID_LOAD_CHAT_THREAD_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        json root;
        auto uid = req_json["uid"].get<int>();
        auto last_id = req_json["thread_id"].get<int>();
        std::cout << "get uid  threads  " << uid << std::endl;
        root["error"] = ErrorCodes::Success;
        root["uid"] = uid;
        std::vector<std::shared_ptr<ChatThreadInfo>> threads;
        bool load_more = false;
        int next_last_id = 0;
        bool res = MysqlMgr::GetInstance()->GetUserThreads(uid, last_id, 10, threads, load_more, next_last_id);
        if (!res) {
            root["error"] = ErrorCodes::UidInvalid;
            session->Send(root.dump(), ID_LOAD_CHAT_THREAD_RSP);
            return;
        }
        root["load_more"] = load_more;
        root["next_last_id"] = (int)next_last_id;
        for (const auto& thread : threads) {
            json thread_obj;
            thread_obj["thread_id"] = thread->_thread_id;
            thread_obj["type"] = thread->_type;
            thread_obj["user1_id"] = thread->_user1_id;
            thread_obj["user2_id"] = thread->_user2_id;
            root["threads"].emplace_back(thread_obj);
        }
        session->Send(root.dump(), ID_LOAD_CHAT_THREAD_RSP);
    };

    _fun_callbacks[ID_CREATE_PRIVATE_CHAT_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        json root;
        root["error"] = ErrorCodes::Success;
        auto uid = req_json["uid"].get<int>();
        auto other_id = req_json["other_id"].get<int>();
        root["uid"] = uid;
        root["other_id"] = other_id;
        int thread_id = 0;
        bool res = MysqlMgr::GetInstance()->CreatePrivateChat(uid, other_id, thread_id);
        if (!res) {
            root["error"] = ErrorCodes::CREATE_CHAT_FAILED;
            return;
        }
        root["thread_id"] = thread_id;
        session->Send(root.dump(), ID_CREATE_PRIVATE_CHAT_RSP);
    };

    _fun_callbacks[ID_LOAD_CHAT_MSG_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        json root;
        auto thread_id = req_json["thread_id"].get<int>();
        auto message_id = req_json["message_id"].get<int>();

        root["error"] = ErrorCodes::Success;
        root["thread_id"] = thread_id;

        int page_size = 10;
        std::shared_ptr<PageResult> res = MysqlMgr::GetInstance()->LoadChatMsg(thread_id, message_id, page_size);
        if (!res) {
            root["error"] = ErrorCodes::LOAD_CHAT_FAILED;
            session->Send(root.dump(), ID_LOAD_CHAT_MSG_RSP);
            return;
        }

        root["last_message_id"] = res->next_cursor;
        root["load_more"] = res->load_more;
        for (auto& chat : res->messages) {
            json chat_data;
            chat_data["sender"] = chat.sender_id;
            chat_data["msg_id"] = chat.message_id;
            chat_data["thread_id"] = chat.thread_id;
            chat_data["unique_id"] = chat.unique_id;
            chat_data["msg_content"] = chat.content;
            chat_data["chat_time"] = chat.chat_time;
            chat_data["status"] = chat.status;
            chat_data["type"] = chat.type;
            root["chat_datas"].emplace_back(chat_data);
        }

        session->Send(root.dump(), ID_LOAD_CHAT_MSG_RSP);
    };


    _fun_callbacks[ID_IMG_CHAT_MSG_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        auto uid = req_json["fromuid"].get<int>();
        auto touid = req_json["touid"].get<int>();

        auto md5 = req_json["md5"].get<std::string>();
        auto unique_name = req_json["name"].get<std::string>();
        auto token = req_json["token"].get<std::string>();
        auto unique_id = req_json["unique_id"].get<std::string>();
        auto chat_time = req_json.contains("chat_time")
            ? req_json["chat_time"].get<std::string>() : getCurrentTimestamp();
        auto status = req_json.contains("status")
            ? req_json["status"].get<int>() : MsgStatus::UN_UPLOAD;

        json  rtvalue;
        rtvalue["error"] = ErrorCodes::Success;

        rtvalue["fromuid"] = uid;
        rtvalue["touid"] = touid;
        auto thread_id = req_json["thread_id"].get<int>();
        rtvalue["thread_id"] = thread_id;
        rtvalue["md5"] = md5;
        rtvalue["unique_name"] = unique_name;
        rtvalue["unique_id"] = unique_id;
        rtvalue["chat_time"] = chat_time;
        rtvalue["status"] = status;

        auto timestamp = getCurrentTimestamp();
        auto chat_msg = std::make_shared<ChatMessage>();
        chat_msg->chat_time = timestamp;
        chat_msg->sender_id = uid;
        chat_msg->recv_id = touid;
        chat_msg->unique_id = unique_id;
        chat_msg->thread_id = thread_id;
        chat_msg->content = unique_name;
        chat_msg->status = MsgStatus::UN_UPLOAD;
        chat_msg->type = MsgType::MSG_TYPE_IMG;

        //插入数据库
        MysqlMgr::GetInstance()->AddChatMsg(chat_msg);
        rtvalue["message_id"] = chat_msg->message_id;
        session->Send(rtvalue.dump(), ID_IMG_CHAT_MSG_RSP);
    };
    _fun_callbacks[ID_FILE_CHAT_MSG_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        auto uid = req_json["fromuid"].get<int>();
        auto touid = req_json["touid"].get<int>();

        auto md5 = req_json["md5"].get<std::string>();
        auto total_size = req_json.value("total_size", 0);
        auto unique_name = req_json["name"].get<std::string>();
        auto token = req_json["token"].get<std::string>();
        auto unique_id = req_json["unique_id"].get<std::string>();
        auto chat_time = req_json.contains("chat_time")
            ? req_json["chat_time"].get<std::string>() : getCurrentTimestamp();
        auto status = req_json.contains("status")
            ? req_json["status"].get<int>() : MsgStatus::UN_UPLOAD;

        json rtvalue;
        rtvalue["error"] = ErrorCodes::Success;
        rtvalue["fromuid"] = uid;
        rtvalue["touid"] = touid;
        auto thread_id = req_json["thread_id"].get<int>();
        rtvalue["thread_id"] = thread_id;
        rtvalue["md5"] = md5;
        rtvalue["unique_name"] = unique_name;
        rtvalue["unique_id"] = unique_id;
        rtvalue["chat_time"] = chat_time;
        rtvalue["status"] = status;
        rtvalue["total_size"] = total_size;

        auto timestamp = getCurrentTimestamp();
        auto chat_msg = std::make_shared<ChatMessage>();
        chat_msg->chat_time = timestamp;
        chat_msg->sender_id = uid;
        chat_msg->recv_id = touid;
        chat_msg->unique_id = unique_id;
        chat_msg->thread_id = thread_id;
        chat_msg->content = unique_name;
        chat_msg->status = MsgStatus::UN_UPLOAD;
        chat_msg->type = MsgType::MSG_TYPE_FILE;

        MysqlMgr::GetInstance()->AddChatMsg(chat_msg);
        rtvalue["message_id"] = chat_msg->message_id;
        NotifyFileChatMsg(uid, touid, thread_id, chat_msg->message_id,
            unique_id, unique_name, md5, chat_msg->chat_time,
            MsgStatus::UN_UPLOAD, MsgType::MSG_TYPE_FILE, total_size);
        session->Send(rtvalue.dump(), ID_FILE_CHAT_MSG_RSP);
    };

    _fun_callbacks[ID_IMG_CHAT_UPLOAD_FINISH_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        auto from_uid = req_json["fromuid"].get<int>();
        auto to_uid = req_json["touid"].get<int>();
        auto thread_id = req_json["thread_id"].get<int>();
        auto message_id = req_json["message_id"].get<int>();
        auto unique_id = req_json["unique_id"].get<std::string>();
        auto unique_name = req_json["name"].get<std::string>();
        auto md5 = req_json["md5"].get<std::string>();
        auto chat_time = req_json.contains("chat_time")
            ? req_json["chat_time"].get<std::string>() : getCurrentTimestamp();

        MysqlMgr::GetInstance()->UpdateChatMsgStatus(message_id, MsgStatus::UPLOAD);

        json notify;
        notify["error"] = ErrorCodes::Success;
        notify["fromuid"] = from_uid;
        notify["touid"] = to_uid;
        notify["thread_id"] = thread_id;
        notify["message_id"] = message_id;
        notify["unique_id"] = unique_id;
        notify["name"] = unique_name;
        notify["md5"] = md5;
        notify["chat_time"] = chat_time;
        notify["status"] = MsgStatus::UPLOAD;
        notify["type"] = MsgType::MSG_TYPE_IMG;

        auto touid_str = std::to_string(to_uid);
        auto to_ip_key = USERIPPREFIX + touid_str;
        std::string to_ip_value = "";
        bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
        if (!b_ip) {
            return;
        }

        auto &cfg = ConfigMgr::Inst();
        auto self_name = cfg["SelfServer"]["Name"];
        if (to_ip_value == self_name) {
            auto to_session = UserMgr::GetInstance()->getSession(to_uid);
            if (to_session) {
                to_session->Send(notify.dump(), ID_NOTIFY_IMG_CHAT_MSG_REQ);
            }
        } else {
            message::ImageChatMsgReq image_req;
            image_req.set_fromuid(from_uid);
            image_req.set_touid(to_uid);
            image_req.set_thread_id(thread_id);
            image_req.set_message_id(message_id);
            image_req.set_unique_id(unique_id);
            image_req.set_name(unique_name);
            image_req.set_md5(md5);
            image_req.set_chat_time(chat_time);
            image_req.set_status(MsgStatus::UPLOAD);
            image_req.set_type(MsgType::MSG_TYPE_IMG);
            ChatGrpcClient::GetInstance()->NotifyImageChatMsg(to_ip_value, image_req);
        }
    };
    _fun_callbacks[ID_FILE_CHAT_UPLOAD_FINISH_REQ] = [this](std::shared_ptr<CSession> session, const short &msg_id, const std::string & msg_data) {
        json req_json = json::parse(msg_data);
        auto from_uid = req_json["fromuid"].get<int>();
        auto to_uid = req_json["touid"].get<int>();
        auto thread_id = req_json["thread_id"].get<int>();
        auto message_id = req_json["message_id"].get<int>();
        auto unique_id = req_json["unique_id"].get<std::string>();
        auto unique_name = req_json["name"].get<std::string>();
        auto total_size = req_json["total_size"].get<int>();
        auto md5 = req_json["md5"].get<std::string>();
        auto chat_time = req_json.contains("chat_time")
            ? req_json["chat_time"].get<std::string>() : getCurrentTimestamp();

        MysqlMgr::GetInstance()->UpdateChatMsgStatus(message_id, MsgStatus::UPLOAD);
        NotifyFileChatMsg(from_uid, to_uid, thread_id, message_id, unique_id, unique_name, md5, chat_time,
            MsgStatus::UPLOAD, MsgType::MSG_TYPE_FILE, total_size);
    };




}

void LogicSystem::NotifyFileChatMsg(int from_uid, int to_uid, int thread_id, int message_id,
    const std::string& unique_id, const std::string& unique_name,
    const std::string& md5, const std::string& chat_time, int status, int type, int total_size) {
    json notify;
    notify["error"] = ErrorCodes::Success;
    notify["fromuid"] = from_uid;
    notify["touid"] = to_uid;
    notify["thread_id"] = thread_id;
    notify["message_id"] = message_id;
    notify["unique_id"] = unique_id;
    notify["name"] = unique_name;
    notify["md5"] = md5;
    notify["chat_time"] = chat_time;
    notify["status"] = status;
    notify["type"] = type;
    notify["total_size"] = total_size;

    auto touid_str = std::to_string(to_uid);
    auto to_ip_key = USERIPPREFIX + touid_str;
    std::string to_ip_value = "";
    bool b_ip = RedisMgr::GetInstance()->Get(to_ip_key, to_ip_value);
    if (!b_ip) {
        return;
    }

    auto& cfg = ConfigMgr::Inst();
    auto self_name = cfg["SelfServer"]["Name"];
    if (to_ip_value == self_name) {
        auto to_session = UserMgr::GetInstance()->getSession(to_uid);
        if (to_session) {
            to_session->Send(notify.dump(), ID_NOTIFY_IMG_CHAT_MSG_REQ);
        }
    }
    else {
        message::ImageChatMsgReq image_req;
        image_req.set_fromuid(from_uid);
        image_req.set_touid(to_uid);
        image_req.set_thread_id(thread_id);
        image_req.set_message_id(message_id);
        image_req.set_unique_id(unique_id);
        image_req.set_name(unique_name);
        image_req.set_md5(md5);
        image_req.set_chat_time(chat_time);
        image_req.set_status(status);
        image_req.set_type(type);
        image_req.set_total_size(total_size);
        ChatGrpcClient::GetInstance()->NotifyImageChatMsg(to_ip_value, image_req);
    }
}

void LogicSystem::DealMsg() {
    for (;;) {
        std::unique_lock<std::mutex> lock(_mutex);
        _consume.wait(lock, [this]() {
            return !_msg_que.empty() || _b_stop;
        });

        if (_b_stop) {
            while (!_msg_que.empty()) {
                auto& msg_node = _msg_que.front();
                std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
                auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
                if (call_back_iter == _fun_callbacks.end()) {
                    _msg_que.pop();
                    continue;
                }
                try {
                    call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
                        std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
                } catch (const std::exception& e) {
                    std::cerr << "DealMsg handler exception: " << e.what() << std::endl;
                }
                _msg_que.pop();
            }
            return;
        }
        auto &msg_node = _msg_que.front();
        std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
        auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
        if (call_back_iter == _fun_callbacks.end()) {
            _msg_que.pop();
            std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
            continue;
        }
        try {
            call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
                std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
        } catch (const std::exception& e) {
            std::cerr << "DealMsg handler exception: " << e.what() << std::endl;
        }
        _msg_que.pop();
    }
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg) {
    std::unique_lock<std::mutex> unique_lk(_mutex);
    _msg_que.push(msg);
    //鐢?鍙樹负1鍒欏彂閫侀€氱煡淇″彿
    if (_msg_que.size() == 1) {
        unique_lk.unlock();
        _consume.notify_one();
    }
}

bool LogicSystem::GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> user_info) {
    std::string value_str;
    bool b_get = RedisMgr::GetInstance()->Get(base_key, value_str);
    if (b_get) {
        json root = json::parse(value_str);
        user_info->uid = root["uid"].get<int>();
        user_info->username = root["name"].get<std::string>();
        user_info->password = root["pwd"].get<std::string>();
        user_info->email = root["email"].get<std::string>();
        user_info->nick = root["nick"].get<std::string>();
        user_info->desc = root["desc"].get<std::string>();
        user_info->sex = root["sex"].get<int>();
        user_info->icon = root["icon"].get<std::string>();
        std::cout << "user login uid is  " << user_info->uid << " name  is "
            << user_info->username << " pwd is " << user_info->password << " email is " << user_info->email << std::endl;
    }else {
        user_info = MysqlMgr::GetInstance()->GetUserInfoById(uid);
        if (!user_info) {
            return false;
        }
        json redis_root;
        redis_root["uid"] = uid;
        redis_root["pwd"] = user_info->password;
        redis_root["name"] = user_info->username;
        redis_root["email"] = user_info->email;
        redis_root["nick"] = user_info->nick;
        redis_root["desc"] = user_info->desc;
        redis_root["sex"] = user_info->sex;
        redis_root["icon"] = user_info->icon;
        RedisMgr::GetInstance()->Set(base_key, redis_root.dump());
    }
    return true;

}

bool LogicSystem::isPureDigit(std::string uid_str) {
    for (const auto& c : uid_str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

void LogicSystem::GetUserByUid(std::string uid_str, json & rtvalue) {
    rtvalue["error"] = ErrorCodes::Success;
    auto base_key = USER_BASE_INFO + uid_str;
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        json root = json::parse(info_str);
        auto uid = root["uid"].get<int>();
        auto name = root["name"].get<std::string>();
        auto pwd = root["pwd"].get<std::string>();
        auto email = root["email"].get<std::string>();
        auto nick = root["nick"].get<std::string>();
        auto desc = root["desc"].get<std::string>();
        auto sex = root["sex"].get<int>();
        auto icon = root["icon"].get<std::string>();
        std::cout << "user  uid is  " << uid << " name  is "
            << name << " pwd is " << pwd << " email is " << email <<" icon is " << icon << std::endl;

        rtvalue["uid"] = uid;
        rtvalue["pwd"] = pwd;
        rtvalue["name"] = name;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        rtvalue["icon"] = icon;
        return;
    }

    auto uid = std::stoi(uid_str);
    //redis涓病鏈夊垯鏌ヨmysql
    //鏌ヨ鏁版嵁搴?
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MysqlMgr::GetInstance()->GetUserInfoById(uid);
    if (user_info == nullptr) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    //灏嗘暟鎹簱鍐呭鍐欏叆redis缂撳瓨
    json redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["pwd"] = user_info->password;
    redis_root["name"] = user_info->username;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;

    RedisMgr::GetInstance()->Set(base_key, redis_root.dump());

    //杩斿洖鏁版嵁
    rtvalue["uid"] = user_info->uid;
    rtvalue["pwd"] = user_info->password;
    rtvalue["name"] = user_info->username;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
}

void LogicSystem::GetUserByName(std::string name, json& rtvalue)
{
    rtvalue["error"] = ErrorCodes::Success;

    std::string base_key = NAME_INFO + name;

    //浼樺厛鏌edis涓煡璇㈢敤鎴蜂俊鎭?
    std::string info_str = "";
    bool b_base = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (b_base) {
        json root = json::parse(info_str);
        auto uid = root["uid"].get<int>();
        auto name = root["name"].get<std::string>();
        auto pwd = root["pwd"].get<std::string>();
        auto email = root["email"].get<std::string>();
        auto nick = root["nick"].get<std::string>();
        auto desc = root["desc"].get<std::string>();
        auto sex = root["sex"].get<int>();
        auto icon = root["icon"].get<std::string>();
        std::cout << "user  uid is  " << uid << " name  is "
            << name << " pwd is " << pwd << " email is " << email << std::endl;

        rtvalue["uid"] = uid;
        rtvalue["pwd"] = pwd;
        rtvalue["name"] = name;
        rtvalue["email"] = email;
        rtvalue["nick"] = nick;
        rtvalue["desc"] = desc;
        rtvalue["sex"] = sex;
        rtvalue["icon"] = icon;
        return;
    }

    //redis涓病鏈夊垯鏌ヨmysql
    //鏌ヨ鏁版嵁搴?
    std::shared_ptr<UserInfo> user_info = nullptr;
    user_info = MysqlMgr::GetInstance()->GetUserInfoByName(name);
    if (user_info == nullptr) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    //灏嗘暟鎹簱鍐呭鍐欏叆redis缂撳瓨
    json redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["pwd"] = user_info->password;
    redis_root["name"] = user_info->username;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;

    RedisMgr::GetInstance()->Set(base_key, redis_root.dump());

    //杩斿洖鏁版嵁
    rtvalue["uid"] = user_info->uid;
    rtvalue["pwd"] = user_info->password;
    rtvalue["name"] = user_info->username;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
}




