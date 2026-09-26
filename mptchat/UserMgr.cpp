//
// Created by mpt on 2026/7/9.
//

#include "ClickedOnceLabel.h"
#include "usermgr.h"
#include "userdata.h"
#include <QDir>
#include <QStandardPaths>
#include <QPixmap>
#include <QLabel>
#include <QFileInfo>
#include "TCPFileMgr.h"
UserMgr::~UserMgr()
{

}

UserMgr::UserMgr(): _user_info(nullptr),  _last_thread_id(0), _cur_load_chat_index(0)
{

}

void UserMgr::Clear()
{
    std::lock_guard<std::mutex> down_lock(_down_load_mtx);
    std::lock_guard<std::mutex> file_lock(_file_msg_mtx);

    _user_info = nullptr;
    _token.clear();
    _last_thread_id = 0;
    _cur_load_chat_index = 0;

    _apply_list.clear();
    _friend_list.clear();
    _friend_map.clear();
    _group_member_infos.clear();
    _friend_chat_msgs.clear();
    _chat_map.clear();
    _uid_to_thread_id.clear();
    _chat_thread_ids.clear();

    _name_files.clear();
    _file_infos.clear();
    _name_to_download_info.clear();
    _avatar_requested.clear();
    _file_msgs.clear();
    _name_to_reset_labels.clear();
}


void UserMgr::SetUserInfo(std::shared_ptr<UserInfo> user_info) {
    _user_info = user_info;
}

void UserMgr::SetToken(QString token)
{
    _token = token;
}

int UserMgr::GetUid()
{
    return _user_info ? _user_info->_uid : 0;
}

QString UserMgr::GetName()
{
    return _user_info->_name;
}

QString UserMgr::GetNick()
{
    return _user_info->_nick;
}

QString UserMgr::GetDesc()
{
    return _user_info->_desc;
}

QString UserMgr::GetIcon()
{
    return _user_info->_icon;
}

void UserMgr::AddApplyList(std::shared_ptr<ApplyInfo> app)
{
    _apply_list.push_back(app);
}

bool UserMgr::AlreadyApply(int uid)
{
    for(auto& apply: _apply_list){
        if(apply->_uid == uid){
            return true;
        }
    }

    return false;
}

void UserMgr::AppendApplyList(QJsonArray array)
{
    for (const QJsonValue &value : array) {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto status = value["status"].toInt();
        auto info = std::make_shared<ApplyInfo>(uid, name,
                           desc, icon, nick, sex, status);
        _apply_list.push_back(info);
    }
}

void UserMgr::AppendFriendList(QJsonArray array) {
    for (const QJsonValue& value : array) {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto back = value["back"].toString();

        auto info = std::make_shared<UserInfo>(uid, name,
            nick, icon, sex, desc, back);
        _friend_list.push_back(info);
        _friend_map.insert(uid, info);
    }
}

std::vector<std::shared_ptr<ApplyInfo> > UserMgr::GetApplyList()
{
    return _apply_list;
}

std::vector<std::shared_ptr<UserInfo> > UserMgr::GetFriendList()
{
    return _friend_list;
}

std::shared_ptr<UserInfo> UserMgr::GetUserInfo() {
    return _user_info;
}

void UserMgr::AddFileInfoByMD5(QString md5, std::shared_ptr<FileInfo> info) {
    _file_infos.insert(md5, info);
}

std::shared_ptr<FileInfo> UserMgr::GetFileInfoByMD5(QString md5) {
    auto it = _file_infos.find(md5);
    if (it == _file_infos.end()) {
        return nullptr;
    }
    return it.value();
}

void UserMgr::AddNameFile(QString name, std::shared_ptr<FileInfo> info) {
    if (!info) {
        return;
    }
    _name_files.insert(name, info);
    if (!info->_md5.isEmpty()) {
        _file_infos.insert(info->_md5, info);
    }
}

std::shared_ptr<FileInfo> UserMgr::GetNameFile(QString name) {
    auto it = _name_files.find(name);
    if (it == _name_files.end()) {
        return nullptr;
    }
    return it.value();
}

bool UserMgr::CheckFriendById(int uid) {
    auto it = _friend_map.find(uid);
    if(it == _friend_map.end()) return false;
    return true;
}

void UserMgr::AddFriend(std::shared_ptr<AuthInfo> user) {
    auto friend_info = std::make_shared<UserInfo>(user);
    _friend_map[user->_uid] = friend_info;
    _friend_list.push_back(friend_info);
}

void UserMgr::AddFriend(std::shared_ptr<AuthRsp> user) {
    auto friend_info = std::make_shared<UserInfo>(user);
    _friend_map[user->_uid] = friend_info;
    _friend_list.push_back(friend_info);
}

std::shared_ptr<UserInfo> UserMgr::GetFriendById(int uid) {
    auto iter = _friend_map.find(uid);
    if(iter == _friend_map.end()) return nullptr;
    return *iter;
}

std::shared_ptr<UserInfo> UserMgr::GetUserInfoById(int uid) {
    auto friend_info = GetFriendById(uid);
    if (friend_info) {
        return friend_info;
    }

    auto iter = _group_member_infos.find(uid);
    if (iter == _group_member_infos.end()) {
        return nullptr;
    }
    return iter.value();
}

void UserMgr::AddGroupMemberInfo(std::shared_ptr<UserInfo> info) {
    if (!info || info->_uid <= 0) {
        return;
    }
    _group_member_infos[info->_uid] = info;
}

void UserMgr::AppendFriendChatMsg(int friend_uid, QJsonArray contents) {
    auto &msgs = _friend_chat_msgs[friend_uid];
    for (const QJsonValue &val : contents) {
        if (val.isObject()) {
            msgs.append(val.toObject());
        }
    }
}

QVector<QJsonObject> UserMgr::GetFriendChatMsgs(int friend_uid) {
    auto it = _friend_chat_msgs.find(friend_uid);
    if (it == _friend_chat_msgs.end()) return {};
    return it.value();
}

void UserMgr::AddChatThreadData(std::shared_ptr<ChatThreadData> thread) {
    _chat_map[thread->GetThreadId()] = thread;
    _chat_thread_ids.push_back(thread->GetThreadId());
    if (thread->GetOtherId()) {
        _uid_to_thread_id[thread->GetOtherId()] = thread->GetThreadId();
    }

}

void UserMgr::SetLastChatThreadId(int last_thread_id) {
    _last_thread_id = last_thread_id;
}

int UserMgr::GetChatThreadByUid(int uid) {
    auto iter = _uid_to_thread_id.find(uid);
    if (iter == _uid_to_thread_id.end()) return -1;
    return iter.value();
}

std::shared_ptr<ChatThreadData> UserMgr::GetChatThreadDataByThreadId(int thread_id) {
    auto iter = _chat_map.find(thread_id);
    if (iter == _chat_map.end()) return nullptr;
    return iter.value();
}

std::shared_ptr<ChatThreadData> UserMgr::GetCurLoadData()
{
    if (_cur_load_chat_index >= _chat_thread_ids.size()) {
        return nullptr;
    }

    auto iter = _chat_map.find(_chat_thread_ids[_cur_load_chat_index]);
    if (iter == _chat_map.end()) {
        return nullptr;
    }

    return iter.value();
}

std::shared_ptr<ChatThreadData> UserMgr::GetNextLoadData() {
    _cur_load_chat_index++;
    if (_cur_load_chat_index >= _chat_thread_ids.size()) {
        return nullptr;
    }

    auto iter = _chat_map.find(_chat_thread_ids[_cur_load_chat_index]);
    if (iter == _chat_map.end()) {
        return nullptr;
    }

    return iter.value();
}

void UserMgr::SetIcon(QString name) {
    _user_info->_icon = name;
}

QString UserMgr::GetAvatarLocalPath(const QString &iconFileName)
{
    if (iconFileName.isEmpty()) {
        return QString();
    }

    const QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString avatar_dir = QDir(storage_dir).filePath("avatars");
    return QDir(avatar_dir).filePath(iconFileName);
}

void UserMgr::SetLabelAvatar(QLabel *label, const QString &iconFileName)
{
    if (!label || iconFileName.isEmpty()) {
        return;
    }

    const QString path = GetAvatarLocalPath(iconFileName);
    QPixmap pix(path);
    if (!pix.isNull()) {
        QPixmap scaled = pix.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        label->setPixmap(scaled);
        label->setScaledContents(true);
    } else {
        label->setPixmap(QPixmap());
        UserMgr::instance()->AddLabelToReset(path, label);
    }
}
bool UserMgr::EnsureAvatarDownloaded(const QString &iconFileName, int owner_uid)
{
    if (iconFileName.isEmpty()) {
        return false;
    }

    const QString path = GetAvatarLocalPath(iconFileName);
    if (path.isEmpty()) {
        return false;
    }

    if (QFileInfo::exists(path)) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(_down_load_mtx);
        if (_avatar_requested.contains(iconFileName)) {
            return false;
        }
        _avatar_requested.insert(iconFileName);
    }

    auto download_info = std::make_shared<DownloadInfo>(iconFileName, path, owner_uid);
    AddDownloadFile(iconFileName, download_info);
    TCPFileMgr::instance()->SendDownloadInfo(download_info);
    return false;
}

bool UserMgr::IsDownLoading(QString name) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    auto iter = _name_to_download_info.find(name);
    if (iter == _name_to_download_info.end()) {
        return false;
    }

    return true;
}

void UserMgr::AddLabelToReset(QString path, QLabel* label)
{
    _name_to_reset_labels[path].append(label);
}


void UserMgr::AddDownloadFile(QString name, std::shared_ptr<DownloadInfo> file_info) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    _name_to_download_info[name] = file_info;
}

void UserMgr::ResetLabelIcon(QString path)
{
    auto iter =  _name_to_reset_labels.find(path);
    if (iter == _name_to_reset_labels.end()) {
        return;
    }

    for (auto ele_iter = iter.value().begin(); ele_iter != iter.value().end(); ele_iter++) {
        QPixmap pixmap(path); // 加载上传的头像图片
        if (!pixmap.isNull()) {
            QPixmap scaledPixmap = pixmap.scaled((*ele_iter)->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            (*ele_iter)->setPixmap(scaledPixmap);
            (*ele_iter)->setScaledContents(true);
        }
        else {
            qWarning() << "无法加载上传的头像：" << path;
        }
    }

    _name_to_reset_labels.erase(iter);
}

void UserMgr::RmvDownloadFile(QString name) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    auto iter = _name_to_download_info.find(name);
    if (iter != _name_to_download_info.end()) {
        _name_to_download_info.erase(iter);
        _avatar_requested.remove(name);
    }
}

std::shared_ptr<DownloadInfo> UserMgr::GetDownloadInfo(QString name) {
    std::lock_guard<std::mutex> lock(_down_load_mtx);
    auto iter = _name_to_download_info.find(name);
    if (iter != _name_to_download_info.end()) {
        return iter.value();
    }
    return nullptr;
}

void UserMgr::AddTransFile(QString name, std::shared_ptr<MsgInfo> msg_info) {
    std::lock_guard<std::mutex> lock(_file_msg_mtx);
    auto iter = _file_msgs.find(name);
    if (iter == _file_msgs.end()) {
        _file_msgs[name] = msg_info;
    }
}

std::shared_ptr<MsgInfo> UserMgr::GetTransFileByName(QString name) {
    std::lock_guard<std::mutex> lock(_file_msg_mtx);
    if (_file_msgs.find(name) != _file_msgs.end()) {
        return _file_msgs[name];
    }
    return nullptr;
}



