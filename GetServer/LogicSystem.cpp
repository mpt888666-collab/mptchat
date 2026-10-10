//
// Created by mpt on 2026/6/29.
//

#include "LogicSystem.h"

#include "RedisMgr.h"
#include <iostream>
#include <memory>
#include <string>
#include <utility>

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    _get_handlers.insert(std::make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
    _post_handlers.insert(std::make_pair(url, handler));
}

bool LogicSystem::HandleGet(std::string url, std::shared_ptr<HttpConnection> conn) {
    if (_get_handlers.find(url) == _get_handlers.end()) {
        return false;
    }
    _get_handlers[url](conn);
    return true;
}

bool LogicSystem::HandlePost(std::string url, std::shared_ptr<HttpConnection> conn) {
    if (_post_handlers.find(url) == _post_handlers.end()) {
        return false;
    }
    _post_handlers[url](conn);
    return true;
}

LogicSystem::LogicSystem() {
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
       ostream(connection->_response.body()) << "receive get_test req";
   });

    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        connection->_response.set(boost::beast::http::field::content_type, "application/json");
        json root;
        try {
            json req_json = json::parse(body_str);
            std::string email = req_json.value("email", std::string{});
            message::GetVarifyRsp rsp = VarifyGrpcClient::GetInstance()->GetVarifyCode(email);
            std::cout << "email is : "<< email << std::endl;
            root["email"] = email;
            root["error"] = rsp.error();
            std::cout << rsp.error() << std::endl;
            auto variety_code = rsp.code();
            RedisMgr::GetInstance()->Set("email", variety_code);
            std::cout << "Code is : "<< rsp.code() << std::endl;
            auto json_str = root.dump();
            boost::beast::ostream(connection->_response.body()) << json_str;
            connection->_response.prepare_payload();
        }catch (const json::exception& e) {
            // JSON格式错误
            json err;
            err["error"] = ErrorCodes::Error_Json;
            boost::beast::ostream(connection->_response.body()) << err.dump();
            connection->_response.prepare_payload();
        }
    });

    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        connection->_response.set(boost::beast::http::field::content_type, "application/json");
        json root;
        try {
            json req_json = json::parse(body_str);
            std::string email = req_json.value("email", std::string{});
            std::string variety_code;
            bool b_get_variety_code = RedisMgr::GetInstance()->Get("code_"+email, variety_code);
            if (!b_get_variety_code) {
                std::cout << "get varify code not expired" << std::endl;
                root["error"] = ErrorCodes::VarifyExpired;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }
            std::string user_variety_code = req_json.value("variety_code", std::string{});
            if (variety_code != user_variety_code) {
                std::cout << "varify code error" << std::endl;
                root["error"] = ErrorCodes::VarifyCodeErr;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }

            std::string user = req_json.value("user", std::string{});

            //查找邮箱是否注册过
            bool b_find_user = MysqlMgr::GetInstance()->IsEmailUsed(email);
            if (b_find_user) {
                std::cout << "sql user exist" << std::endl;
                root["error"] = ErrorCodes::UserAlreadyExist;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }

            root["user"] = req_json.value("user", std::string{});
            root["error"] = ErrorCodes::Success;
            root["email"] = req_json.value("email", std::string{});
            root["variety_code"] = req_json.value("variety_code", std::string{});
            root["confirm"] = req_json.value("confirm", std::string{});
            root["passwd"] = req_json.value("passwd", std::string{});
            //将用户信息存入数据库

            bool b_register = MysqlMgr::GetInstance()->RegisterUser(user, req_json.value("passwd", std::string{}), email);
            if (!b_register) {
                std::cout << " register user failed" << std::endl;
                root["error"] = ErrorCodes::RegisterFailed;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }
            std::cout << " register user success" << std::endl;
            root["error"] = ErrorCodes::Success;
            boost::beast::ostream(connection->_response.body()) << root.dump();

        }catch (const json::exception& e) {
            std::cout << "json param error" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            boost::beast::ostream(connection->_response.body()) << root.dump();
            return;
        }
    });

    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        std::string data = boost::beast::buffers_to_string(connection->_request.body().data());
        json root;
        json req_json;
        try {
            req_json = json::parse(data);
            std::string email = req_json.value("email", std::string{});
            std::string user = req_json.value("user", std::string{});
            std::string variety_code = req_json.value("variety_code", std::string{});
            std::string pass = req_json.value("passwd", std::string{});

            bool b_get_variety_code = RedisMgr::GetInstance()->Get("code_"+email, variety_code);
            if (!b_get_variety_code) {
                std::cout << "get varify code not expired" << std::endl;
                root["error"] = ErrorCodes::VarifyExpired;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }
            std::string user_variety_code = req_json.value("variety_code", std::string{});
            if (variety_code != user_variety_code) {
                std::cout << "varify code error" << std::endl;
                root["error"] = ErrorCodes::VarifyCodeErr;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }

            bool b_find_user = MysqlMgr::GetInstance()->IsEmailUsed(email);
            if (!b_find_user) {
                std::cout << "sql user not exist" << std::endl;
                root["error"] = ErrorCodes::UserNotExist;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }
            //进行数据库修改密码操作
            bool b_modify_pass = MysqlMgr::GetInstance()->ModifyPasswordByUserEmail(user, email, pass);
            if (!b_modify_pass) {
                std::cout << "modify password error" << std::endl;
                root["error"] = ErrorCodes::ModifyPassErr;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }
            std::cout << "succeed to update password" << pass << std::endl;
            root["user"] = req_json.value("user", std::string{});
            root["error"] = ErrorCodes::Success;
            root["email"] = req_json.value("email", std::string{});
            root["variety_code"] = req_json.value("variety_code", std::string{});;
            root["passwd"] = req_json.value("passwd", std::string{});

            boost::beast::ostream(connection->_response.body()) << root.dump();
        }catch (const json::exception& e) {
            std::cout << "json param error" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            boost::beast::ostream(connection->_response.body()) << root.dump();
            return;
        }
    });

    RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        std::string data = boost::beast::buffers_to_string(connection->_request.body().data());
        json root;
        json req_json;
        try {
            req_json = json::parse(data);
            std::string email = req_json.value("email", std::string{});
            std::string user = req_json.value("user", std::string{});
            std::string pass = req_json.value("passwd", std::string{});

            bool b_find_user = MysqlMgr::GetInstance()->IsEmailUsed(email);
            if (!b_find_user) {
                std::cout << "sql user not exist" << std::endl;
                root["error"] = ErrorCodes::UserNotExist;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }

            UserInfo userInfo{};
            bool b_modify_login = MysqlMgr::GetInstance()->CheckUserLogin(user, email, pass, userInfo);
            if (!b_modify_login) {
                std::cout << "modify login  error" << std::endl;
                root["error"] = ErrorCodes::ModifyLoginErr;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }
            auto reply = StatusGrpcClient::GetInstance()->GetChatServer(userInfo.uid);
            if (reply.error()) {
                std::cout << " grpc get chat server failed, error is " << reply.error()<< std::endl;
                root["error"] = ErrorCodes::RPCGetFailed;
                boost::beast::ostream(connection->_response.body()) << root.dump();
                return;
            }

            std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
            root["error"] = 0;
            root["user"] = user;
            root["uid"] = userInfo.uid;
            root["token"] = reply.token();
            root["chat_host"] = reply.chat_host();
            root["chat_port"] = reply.chat_port();
            root["res_host"] = reply.res_host();
            root["res_port"] = reply.res_port();
            std::cout << "this port is " << reply.chat_port() << std::endl;
            std::cout << reply.token() << std::endl;
            boost::beast::ostream(connection->_response.body()) << root.dump();
        }catch (const json::exception& e) {
            std::cout << "json param error" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            boost::beast::ostream(connection->_response.body()) << root.dump();
            return;
        }
    });
}

LogicSystem::~LogicSystem() {

}

