//
// Created by mpt on 2026/7/29.
//

#ifndef CHATSERVER_DATA_H
#define CHATSERVER_DATA_H
#include <string>
#include <vector>
// struct UserInfo {
//     int uid;
//     std::string username;
//     std::string password;
//     std::string email;
//     std::string nick;
//     std::string desc;
//     std::string icon;
//     int sex;
// };

struct UserInfo {
    UserInfo():uid(0), username(""),password(""),email(""),nick(""),desc(""),icon(""), sex(0), back("") {}
    int uid;
    std::string username;
    std::string password;
    std::string email;
    std::string nick;
    std::string desc;
    std::string icon;
    int sex;
    std::string back;
};

struct ApplyInfo {
    ApplyInfo() : _uid(0), _sex(0), _status(0) {}
    ApplyInfo(int uid, std::string name, std::string desc,
        std::string icon, std::string nick, int sex, int status)
        :_uid(uid),_name(name),_desc(desc),
        _icon(icon),_nick(nick),_sex(sex),_status(status){}

    int _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int _sex;
    int _status;
};

struct ChatThreadInfo {
    int _thread_id;
    std::string _type;     // "private" or "group"
    int _user1_id;    // 私聊时对应 private_chat.user1_id；群聊时设为 0
    int _user2_id;    // 私聊时对应 private_chat.user2_id；群聊时设为 0
};

struct ChatMessage {
    int message_id;
    int thread_id;
    int sender_id;
    int recv_id;
    std::string unique_id;
    std::string content;
    std::string chat_time;
    int status;
};

struct PageResult {
    std::vector<ChatMessage> messages;
    bool load_more;
    int next_cursor; // 本页最后一条message_id，用于下次查询
};

#endif //CHATSERVER_DATA_H