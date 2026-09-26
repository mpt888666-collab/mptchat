//
// Created by mpt on 2026/9/21.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ChatFriendInfoShow.h" resolved

#include "chatfriendinfoshow.h"
#include "ui_ChatFriendInfoShow.h"
#include "UserMgr.h"

ChatFriendInfoShow::ChatFriendInfoShow(QWidget *parent) : QDialog(parent), ui(new Ui::ChatFriendInfoShow) {
    ui->setupUi(this);
}

ChatFriendInfoShow::~ChatFriendInfoShow() {
    delete ui;
}

void ChatFriendInfoShow::SetPrivateChatInfo(const QString &icon) {
    _icon = icon;
    UserMgr::SetLabelAvatar(ui->icon_label, icon);
}
