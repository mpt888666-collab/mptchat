//
// Created by mpt on 2026/7/14.
//

#ifndef MPTCHAT_USERDATA_H
#define MPTCHAT_USERDATA_H
#include <QWidget>
#include <utility>
#include "global.h"
#include <QJsonObject>
#include <QMap>
#include <memory>
#include <QJsonArray>
#include <QString>
#include <QVector>

struct DownloadInfo {
    DownloadInfo() = default;
    DownloadInfo(QString name, QString client_path, int owner_uid = 0) : _name(name), _seq(0), _client_path(client_path), _owner_uid(owner_uid){}

    QString _name;
    int _seq{};
    int _owner_uid = 0;
    QString _client_path;
    qint64 _total_size = 0;
    qint64 _current_size = 0;
};
struct FileInfo {
    FileInfo() = default;
    FileInfo(QString md5, QString path, QString name)
        : _md5(md5), _file_path(path), _file_name(name) {}
    QString filePath() const { return _file_path; }
    QString fileName() const { return _file_name; }
    QString _md5;
    QString _file_path;
    QString _file_name;
};

class SearchInfo {
public:
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex, QString icon);
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex);
    int _uid;
    QString _name;
    QString _nick;
    QString _desc;
    int _sex;
    QString _icon;
};

class TextChatData;
struct AuthInfo {
    AuthInfo(int uid, QString name,
             QString nick, QString icon, int sex):
        _uid(uid), _name(name), _nick(nick), _icon(icon),
        _sex(sex), _thread_id(0){}

    void SetChatDatas(std::vector<std::shared_ptr<TextChatData>> _chat_datas);
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    int _thread_id;
    std::vector<std::shared_ptr<TextChatData>> _chat_datas;
};

struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name,
            QString peer_nick, QString peer_icon, int peer_sex)
        :_uid(peer_uid),_name(peer_name),_nick(peer_nick),
          _icon(peer_icon),_sex(peer_sex),_thread_id(0)
    {

    }


    void SetChatDatas(std::vector<std::shared_ptr<TextChatData>> _chat_datas);
    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    int _thread_id;
    std::vector<std::shared_ptr<TextChatData>> _chat_datas;
};

struct UserInfo {
    UserInfo(int uid, QString name, QString nick, QString icon, int sex, const QString& last_msg = "", QString desc=""):
        _uid(uid),_name(std::move(name)),_nick(std::move(nick)),_icon(std::move(icon)),_sex(sex),_desc(std::move(desc)){}

    UserInfo(const int uid, QString name, QString icon):
    _uid(uid), _name(std::move(name)), _icon(std::move(icon)),_nick(_name),
    _sex(0),_desc(""){

    }

    UserInfo(std::shared_ptr<AuthInfo> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_desc(""){}

    UserInfo(std::shared_ptr<AuthRsp> auth):
        _uid(auth->_uid),_name(auth->_name),_nick(auth->_nick),
        _icon(auth->_icon),_sex(auth->_sex),_desc(""){}

    UserInfo(std::shared_ptr<SearchInfo> search_info):
       _uid(search_info->_uid),_name(search_info->_name),_nick(search_info->_nick),
   _icon(search_info->_icon),_sex(search_info->_sex), _desc(search_info->_desc){

    }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _desc;
};

class AddFriendApply {
public:
    AddFriendApply(int from_uid, QString name, QString desc,
                   QString icon, QString nick, int sex);
    int _from_uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int     _sex;
};

struct ApplyInfo {
    ApplyInfo(int uid, QString name, QString desc,
        QString icon, QString nick, int sex, int status)
        :_uid(uid),_name(name),_desc(desc),
        _icon(icon),_nick(nick),_sex(sex),_status(status){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        :_uid(addinfo->_from_uid),_name(addinfo->_name),
          _desc(addinfo->_desc),_icon(addinfo->_icon),
          _nick(addinfo->_nick),_sex(addinfo->_sex),
          _status(0)
    {}
    void SetIcon(QString head){
        _icon = head;
    }
    int _uid;
    QString _name;
    QString _desc;
    QString _icon;
    QString _nick;
    int _sex;
    int _status;
};


class ChatDataBase {
public:
    virtual ~ChatDataBase() = default;
    ChatDataBase(int msg_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,
        QString content,int _send_uid, int status, QString chat_time );
    ChatDataBase(QString unique_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,
        QString content, int send_uid, int status, QString chat_time);
    ChatDataBase(int msg_id, QString unique_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,
        QString content, int send_uid, int status, QString chat_time);
    int GetMsgId() { return _msg_id; }
    int GetThreadId() { return _thread_id; }
    ChatFormType GetFormType() { return _form_type; }
    ChatMsgType GetMsgType() { return _msg_type; }
    QString GetContent() { return _content; }
    int GetSendUid() { return _send_uid; }
    QString GetMsgContent(){return _content;}
    // void SetUniqueId(int unique_id);
    QString GetUniqueId();
    int GetStatus() { return _status; }
    void SetMsgId(int msg_id) { _msg_id = msg_id; }
    void SetStatus(int status) { _status = status; }
private:
    //客户端本地唯一标识
    QString _unique_id;
    //消息id
    int _msg_id;
    //会话id
    int _thread_id;
    //群聊还是私聊
    ChatFormType _form_type;
    //文本信息为0，图片为1，文件为2
    ChatMsgType _msg_type;
    QString _content;
    //发送者id
    int _send_uid;
    //状态
    int _status;
    //聊天时间
    QString _chat_time;
};

class TextChatData : public ChatDataBase {
public:

    TextChatData(int msg_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,  QString content,
        int send_uid, int status, QString chat_time="") :
        ChatDataBase(msg_id, thread_id, form_type, msg_type, content, send_uid, status, chat_time)
    {

    }

    TextChatData(QString unique_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type, QString content,
        int send_uid, int status, QString chat_time="") :
        ChatDataBase(unique_id, thread_id, form_type, msg_type, content, send_uid, status, chat_time)
    {

    }

    TextChatData(int msg_id, QString unique_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type, QString content,
        int send_uid, int status, QString chat_time = "") :
        ChatDataBase(msg_id, unique_id, thread_id, form_type, msg_type, content, send_uid, status, chat_time)
    {

    }

};

//聊天线程信息
struct ChatThreadInfo {
    int _thread_id;
    QString _type;     // "private" or "group"
    int _user1_id;    // 私聊时对应 private_chat.user1_id；群聊时设为 0
    int _user2_id;    // 私聊时对应 private_chat.user2_id；群聊时设为 0
    QString _group_name;
    QVector<int> _group_members;
};

//客户端本地存储的聊天线程数据结构
 class ChatThreadData {
 public:
     ChatThreadData(int other_id, int thread_id, int last_msg_id):
         _other_id(other_id), _thread_id(thread_id), _last_msg_id(last_msg_id){}
     void AddMsg(std::shared_ptr<ChatDataBase> msg);
     void MoveMsg(std::shared_ptr<ChatDataBase> msg);
     void SetLastMsgId(int msg_id);
     void SetOtherId(int other_id);
     int  GetOtherId();
     QString GetGroupName();
     void SetGroupMembers(const QVector<int>& members);
     const QVector<int>& GetGroupMembers();
     QMap<int, std::shared_ptr<ChatDataBase>> GetMsgMap();
     int  GetThreadId();
     QMap<int, std::shared_ptr<ChatDataBase>>&  GetMsgMapRef();
     QVector<std::shared_ptr<ChatDataBase>>& GetMsgOrderRef();
     void AppendMsg(int msg_id, std::shared_ptr<ChatDataBase> base_msg);
     void ConfirmMsg(QString unique_id, int msg_id, int status);
     QString GetLastMsg();
     int GetLastMsgId();
     QMap<QString, std::shared_ptr<ChatDataBase>>& GetMsgUnRspRef();
     void AppendUnRspMsg(QString unique_id, std::shared_ptr<ChatDataBase> base_msg);
 private:
     //如果是私聊，则为对方的id；如果是群聊，则为0
     int _other_id;
     int _last_msg_id;
     int _thread_id;
     QString _last_msg;
     //群聊信息,成员列表
     QVector<int> _group_members;
     //群聊名称
     QString _group_name;
     //缓存消息map，抽象为基类，因为会有图片等其他类型消息
     QMap<int, std::shared_ptr<ChatDataBase>>  _msg_map;
     QVector<std::shared_ptr<ChatDataBase>> _msg_order;
     //缓存未回复的消息
     //已发送的消息，还未收到回应的。
     QMap<QString, std::shared_ptr<ChatDataBase>> _msg_unrsp_map;
 };

// class ChatThreadData {
// public:
//     int _user1_id;
//     int _user2_id;
//     int _last_msg_id;
//     int _thread_id;
// };

class TextChatMsg {
public:
    TextChatMsg(int friend_id, int uid, QJsonArray contents): _friend_uid(friend_id), _uid(uid), _contents(contents){}

    int _friend_uid;
    int _uid;
    QJsonArray _contents;


};

class ImgChatData : public ChatDataBase {
public:
    ImgChatData(std::shared_ptr<MsgInfo> msg_info, QString unique_id,
        int thread_id, ChatFormType form_type, ChatMsgType msg_type,
        int send_uid, int status, QString chat_time = ""):
        ChatDataBase(unique_id,thread_id, form_type, msg_type, msg_info->_content_or_url,
            send_uid, status, chat_time), _msg_info(msg_info){

    }

    std::shared_ptr<MsgInfo> _msg_info;
};

class FileChatData : public ChatDataBase {
public:
    FileChatData(std::shared_ptr<MsgInfo> msg_info, QString unique_id,
        int thread_id, ChatFormType form_type, ChatMsgType msg_type,
        int send_uid, int status, QString chat_time = ""):
        ChatDataBase(unique_id,thread_id, form_type, msg_type, msg_info->_content_or_url,
            send_uid, status, chat_time), _msg_info(msg_info){

    }

    std::shared_ptr<MsgInfo> _msg_info;
};


#endif //MPTCHAT_USERDATA_H
