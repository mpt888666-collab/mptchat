//
// Created by mpt on 2026/7/12.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ChatUserWid.h" resolved

#include "chatuserwid.h"
#include "ui_ChatUserWid.h"
#include "userdata.h"
#include "usermgr.h"
#include <QStyle>


ChatUserWid::ChatUserWid(QWidget *parent) : ListItemBase(parent), ui(new Ui::ChatUserWid) {
    ui->setupUi(this);

    SetItemType(ListItemType::CHAT_USER_ITEM);
    _chat_type = ChatFormType::PRIVATE;
}

ChatUserWid::ChatUserWid(ChatFormType type, QWidget *parent) : ListItemBase(parent), ui(new Ui::ChatUserWid) {
    ui->setupUi(this);

    SetItemType(ListItemType::CHAT_USER_ITEM);
    _chat_type = type;
}

ChatUserWid::~ChatUserWid() {
    delete ui;
}

void ChatUserWid::SetInfo(QString name, QString head, QString msg)
{
    _name = name;
    _head = head;
    _msg = msg;
    UserMgr::SetLabelAvatar(ui->ico_label, _head);
    ui->user_name_label->setText(_name);
    ui->user_chat_label->setText(_msg);
}

void ChatUserWid::SetInfo(int uid, QString name, QString head, QString msg)
{
    _uid = uid;
    SetInfo(name, head, msg);
}

void ChatUserWid::SetInfo(std::shared_ptr<UserInfo> user_info)
{
    _uid = user_info->_uid;
    _name = user_info->_name;
    _head = user_info->_icon;
    UserMgr::SetLabelAvatar(ui->ico_label, _head);
    ui->user_name_label->setText(_name);
    ui->user_chat_label->setText(_msg);
}

void ChatUserWid::updateLastMsg(QJsonArray msgs) {
    QJsonObject msgObj;
    QString content;
    for (const auto& msg : msgs) {
        if (msg.isObject()) {
            msgObj = msg.toObject();
            content = msgObj.value("content").toString();
        }
    }
    ui->user_chat_label->setText(content);
}

void ChatUserWid::SetLastMsg(const QString& msg) {
    _msg = msg;
    ui->user_chat_label->setText(msg);
}

void ChatUserWid::SetChatData(std::shared_ptr<ChatThreadData> chat_data) {
    _chat_data = chat_data;
    auto other_id = _chat_data->GetOtherId();
    _uid = other_id;
    if (_chat_type == ChatFormType::PRIVATE) {
        auto other_info = UserMgr::instance()->GetFriendById(other_id);
        if (!other_info) {
            return;
        }
        // 加载图片
        UserMgr::SetLabelAvatar(ui->ico_label, other_info->_icon);

        ui->user_name_label->setText(other_info->_name);

        ui->user_chat_label->setText(chat_data->GetLastMsg());
    }else {

    }

}

std::shared_ptr<ChatThreadData> ChatUserWid::GetChatData()
{
    return _chat_data;
}

void ChatUserWid::SetSelected(bool selected) {
    setProperty("state", selected ? QStringLiteral("selected") : QStringLiteral("normal"));
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void ChatUserWid::SetAvatarList(QVector<QPixmap> &ava) {
    _avatarList = std::move(ava);
}

void ChatUserWid::SetThreadId(int thread_id) {
    _thread_id = thread_id;
}

int ChatUserWid::GetThreadId() const {
    return _thread_id;
}

void ChatUserWid::SetGroupHead(int spacing, int maxShowCount, QVector<QPixmap> &avatarList) {
    ui->ico_label->setSpacing(spacing);
    ui->ico_label->setMaxShowCount(maxShowCount);
    ui->ico_label->setAvatarList(avatarList);
}

void ChatUserWid::SetName(QString name) {
    _name = name;
    ui->user_name_label->setText(_name);
}

void ChatUserWid::SetGroupUids(const QVector<int> &uids) {
    _group_friend_uid = uids;
}

QVector<int> ChatUserWid::GetGroupUids() {
    return _group_friend_uid;
}

