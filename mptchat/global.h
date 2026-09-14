//
// Created by mpt on 2026/6/28.
//

#ifndef MPTCHAT_GLOBAL_H
#define MPTCHAT_GLOBAL_H
#include <functional>
#include <QWidget>
#include <iostream>
#include <string>
#include <QStyle>

#include "MessageTextEdit.h"
#define MAX_FILE_LEN 1024

extern std::function<void(QWidget*)> repolish;
extern QString gate_url_prefix;

constexpr int tip_offset = 5;
constexpr int MIN_APPLY_LABEL_ED_LEN = 60;
const QString add_prefix = "添加: ";

// struct MsgInfo{
//     QString msgFlag;//"text,image,file"
//     QString content;//表示文件和图像的url,文本信息
//     QPixmap pixmap;//文件和图片的缩略图
// };

enum class ChatMsgType {
    TEXT = 0,
    PIC = 1,
    FILE = 2
};

struct MsgInfo {
    MsgInfo(ChatMsgType type, QString content_or_url, QPixmap pix,
        QString unique_name, int total_size, QString md5) :
    _type(type), _content_or_url(content_or_url),_preview_pix(pix), _unique_name(unique_name),
    _current_size(0), _total_size(total_size), _seq(0), _md5(md5){}

    ChatMsgType _type;
    QString _content_or_url;
    QPixmap _preview_pix;
    QString _unique_name;
    int _current_size;
    int _total_size;
    int _seq;
    QString _md5;
    int _msg_id = 0;
    QString _unique_id;
    int _thread_id = 0;
    int _from_uid = 0;
    int _to_uid = 0;
};



enum ReqId {
    ID_GET_VARIFY_CODE = 1001, //鑾峰彇楠岃瘉鐮?
    ID_REG_USER = 1002, //娉ㄥ唽鐢ㄦ埛
    ID_RESET_PWD = 1003, //鏀规柊瀵嗙爜
    ID_LOGIN_USER = 1004,//鐧诲綍鐢ㄦ埛
    ID_CHAT_LOGIN = 1005,
    MSG_CHAT_LOGIN_RSP = 1006,
    ID_SEARCH_USER_REQ = 1007,
    ID_SEARCH_USER_RSP = 1008, //搜索用户回包
    ID_ADD_FRIEND_REQ = 1009,  //添加好友申请
    ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,
    ID_AUTH_FRIEND_REQ = 1013,
    ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ  = 1017,  //文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP  = 1018,  //文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息
    ID_NOTIFY_IMG_CHAT_MSG_REQ = 1020,  //通知用户图片聊天信息
    ID_NOTIFY_OFF_LINE_REQ = 1021, //通知用户下线
    ID_HEART_BEAT_REQ = 1023,      //心跳请求
    ID_HEARTBEAT_RSP = 1024,       //心跳回复
    ID_LOAD_CHAT_THREAD_REQ = 1025, //加载聊天线程请求
    ID_LOAD_CHAT_THREAD_RSP = 1026, //加载聊天线程回复
    ID_CREATE_PRIVATE_CHAT_REQ = 1027, //创建私聊请求
    ID_CREATE_PRIVATE_CHAT_RSP = 1028, //创建私聊回复
    ID_LOAD_CHAT_MSG_REQ = 1029,      //加载聊天消息
    ID_LOAD_CHAT_MSG_RSP = 1030,      //加载聊天消息



    ID_UPLOAD_HEAD_ICON_REQ = 1031,
    ID_UPLOAD_HEAD_ICON_RSP = 1032,
    ID_DOWN_LOAD_FILE_REQ = 1033,
    ID_DOWN_LOAD_FILE_RSP = 1034,
    ID_IMG_CHAT_MSG_REQ = 1035,
    ID_IMG_CHAT_MSG_RSP = 1036,
    ID_IMG_CHAT_UPLOAD_REQ = 1037,
    ID_IMG_CHAT_UPLOAD_RSP = 1038,
    ID_IMG_CHAT_UPLOAD_FINISH_REQ = 1039,
    ID_FILE_CHAT_MSG_REQ = 1040,
    ID_FILE_CHAT_MSG_RSP = 1041,
    ID_FILE_CHAT_UPLOAD_REQ = 1042,
    ID_FILE_CHAT_UPLOAD_RSP = 1043,
    ID_FILE_CHAT_UPLOAD_FINISH_REQ = 1044,
    ID_CREATE_GROUP_CHAT_REQ = 1045,
    ID_CREATE_GROUP_CHAT_RSP = 1046,
    ID_NOTIFY_CREATE_GROUP_CHAT_RSP = 1047,
};

enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1, //Json瑙ｆ瀽澶辫触
    ERR_NETWORK = 2,
};

enum Modules{
    REGISTERMOD = 0,
    RESETMOD = 1,
    LOGINMOD = 2,
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_EMAIL_EMPTY = 2,
    TIP_PWD_ERR = 3,
    TIP_PWD_EMPTY = 4,
    TIP_CONFIRM_ERR = 5,
    TIP_CONFIRM_EMPTY = 6,
    TIP_PWD_CONFIRM = 7,
    TIP_PWD_CONFIRM_EMPTY = 8,
    TIP_VARIFY_ERR = 9,
    TIP_VARIFY_EMPTY = 10,
    TIP_USER_ERR = 11,
    TIP_USER_EMPTY = 12,
};

enum ClickLbState{
    Normal = 0,
    Selected = 1
};

struct ServerInfo {
    int _uid;
    QString _chat_host;
    QString _res_host;
    uint16_t _chat_port;
    uint16_t _res_port;
    QString _token;
};

enum ChatUIMode {
    ChatMode = 0,
    ContactMode,
    SearchMode,
};

enum ListItemType {
    CHAT_USER_ITEM,
    ADD_USER_TIP_ITEM,
    INVALID_ITEM,
    GROUP_TIP_ITEM,
    APPLY_FRIEND_ITEM,
    CONTACT_USER_ITEM,
};

//聊天形式，私聊和群聊
enum class ChatFormType {
    PRIVATE = 0,
    GROUP = 1
};


enum class ChatRole
{

    Self,
    Other
};

enum class MsgStatus
{
    UN_READ,
    READED,
    SEND_FAILED
};

enum class CustomObjType {
    FileCardObj = QTextFormat::UserObject + 1,
};

#define MAX_FILE_LEN 1024
#endif //MPTCHAT_GLOBAL_H
