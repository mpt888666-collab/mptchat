//
// Created by mpt on 2026/7/14.
//

#include "userdata.h"

#include <utility>

SearchInfo::SearchInfo(int uid, QString name,
    QString nick, QString desc, int sex, QString icon):_uid(uid)
  ,_name(name), _nick(nick),_desc(desc),_sex(sex),_icon(icon){
}

SearchInfo::SearchInfo(int uid, QString name,
    QString nick, QString desc, int sex):_uid(uid)
  ,_name(name), _nick(nick),_desc(desc),_sex(sex){
}

AddFriendApply::AddFriendApply(int from_uid, QString name, QString desc,
                               QString icon, QString nick, int sex)
    :_from_uid(from_uid),_name(name),
      _desc(desc),_icon(icon),_nick(nick),_sex(sex)
{

}

ChatDataBase::ChatDataBase(int msg_id, int thread_id, ChatFormType form_type,
    ChatMsgType msg_type, QString content, int send_uid, int status, QString chat_time):_msg_id(msg_id),
_thread_id(thread_id), _form_type(form_type),
_msg_type(msg_type), _content(content), _send_uid(send_uid), _status(status), _chat_time(chat_time){

}

ChatDataBase::ChatDataBase(QString unique_id, int thread_id, ChatFormType form_type,
    ChatMsgType msg_type, QString content, int send_uid, int status, QString chat_time):_unique_id(unique_id),
    _thread_id(thread_id), _form_type(form_type),
    _msg_type(msg_type), _content(content), _send_uid(send_uid),_msg_id(0), _status(status), _chat_time(chat_time)
{

}

ChatDataBase::ChatDataBase(int msg_id, QString unique_id, int thread_id, ChatFormType form_type, ChatMsgType msg_type,
    QString content, int send_uid, int status, QString chat_time):_msg_id(msg_id), _unique_id(unique_id),
    _thread_id(thread_id), _form_type(form_type),
    _msg_type(msg_type), _content(content), _send_uid(send_uid), _status(status), _chat_time(chat_time) {

}

// void ChatDataBase::SetUniqueId(int unique_id)
// {
//     _unique_id = unique_id;
// }

QString ChatDataBase::GetUniqueId()
{
    return _unique_id;
}

void ChatThreadData::AddMsg(std::shared_ptr<ChatDataBase> msg)
{
    if (msg->GetMsgId() > 0 && _msg_map.contains(msg->GetMsgId())) {
        return;
    }
    _msg_map.insert(msg->GetMsgId(), msg);
    _msg_order.push_back(msg);
    _last_msg = msg->GetMsgContent();
    _last_msg_id = msg->GetMsgId();
}

void ChatThreadData::MoveMsg(std::shared_ptr<ChatDataBase> msg) {

    auto iter = _msg_unrsp_map.find(msg->GetUniqueId());
    if (iter == _msg_unrsp_map.end()) {
        AddMsg(msg);
        return;
    }

    auto pending_msg = iter.value();
    pending_msg->SetMsgId(msg->GetMsgId());
    pending_msg->SetStatus(2);
    if (pending_msg->GetMsgId() > 0 && !_msg_map.contains(pending_msg->GetMsgId())) {
        _msg_map.insert(pending_msg->GetMsgId(), pending_msg);
    }
    _msg_unrsp_map.erase(iter);
    _last_msg = pending_msg->GetMsgContent();
    _last_msg_id = pending_msg->GetMsgId();
}

void ChatThreadData::SetLastMsgId(int msg_id)
{
    _last_msg_id = msg_id;
}

void ChatThreadData::SetOtherId(int other_id)
{
    _other_id = other_id;
}

int  ChatThreadData::GetOtherId() {
    return _other_id;
}

void ChatThreadData::SetGroupMembers(const QVector<int>& members)
{
    _group_members = members;
}

const QVector<int>& ChatThreadData::GetGroupMembers()
{
    return _group_members;
}

QString ChatThreadData::GetGroupName()
{
    return _group_name;
}

QMap<int, std::shared_ptr<ChatDataBase>> ChatThreadData::GetMsgMap() {
    return _msg_map;
}

int ChatThreadData::GetThreadId()
{
    return _thread_id;
}

QMap<int, std::shared_ptr<ChatDataBase>>& ChatThreadData::GetMsgMapRef()
{
    return _msg_map;
}

QVector<std::shared_ptr<ChatDataBase>>& ChatThreadData::GetMsgOrderRef()
{
    return _msg_order;
}


void ChatThreadData::AppendMsg(int msg_id, std::shared_ptr<ChatDataBase> base_msg) {
    if (msg_id > 0 && _msg_map.contains(msg_id)) {
        return;
    }
    _msg_map.insert(msg_id, base_msg);

    int insert_pos = 0;
    while (insert_pos < _msg_order.size()) {
        auto existing = _msg_order.at(insert_pos);
        if (existing && existing->GetMsgId() > msg_id) {
            break;
        }
        ++insert_pos;
    }
    _msg_order.insert(insert_pos, base_msg);

     _last_msg = base_msg->GetMsgContent();
     _last_msg_id = msg_id;
}

void ChatThreadData::ConfirmMsg(QString unique_id, int msg_id, int status) {
    auto iter = _msg_unrsp_map.find(unique_id);
    if (iter == _msg_unrsp_map.end()) {
        return;
    }
    auto pending = iter.value();
    if (!pending) {
        _msg_unrsp_map.erase(iter);
        return;
    }
    pending->SetMsgId(msg_id);
    pending->SetStatus(status);
    if (msg_id > 0 && !_msg_map.contains(msg_id)) {
        _msg_map.insert(msg_id, pending);
    }
    _msg_unrsp_map.erase(iter);
    _last_msg = pending->GetMsgContent();
    _last_msg_id = msg_id;
}

 QString ChatThreadData::GetLastMsg()
{
    if (!_msg_order.isEmpty()) {
        auto last = _msg_order.end();
        --last;
        if (*last) {
            return (*last)->GetMsgContent();
        }
    }
    return _last_msg;
}

int ChatThreadData::GetLastMsgId()
{
    return _last_msg_id;
}

void ChatThreadData::AppendUnRspMsg(QString unique_id, std::shared_ptr<ChatDataBase> base_msg)
{
    _msg_unrsp_map.insert(unique_id, base_msg);
    _msg_order.push_back(base_msg);
    _last_msg = base_msg->GetMsgContent();
}

QMap<QString, std::shared_ptr<ChatDataBase>>& ChatThreadData::GetMsgUnRspRef() {
    return _msg_unrsp_map;
}

void AuthInfo::SetChatDatas(std::vector<std::shared_ptr<TextChatData>> chat_datas)
{
    _chat_datas = chat_datas;
    _thread_id = _chat_datas[0]->GetThreadId();
}

void AuthRsp::SetChatDatas(std::vector<std::shared_ptr<TextChatData>> chat_datas) {
    _chat_datas = chat_datas;
    _thread_id = _chat_datas[0]->GetThreadId();
}
