//
// Created by mpt on 2026/7/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ApplyFriendItem.h" resolved

#include "applyfrienditem.h"
#include "ui_ApplyFriendItem.h"
#include <QPushButton>
#include <utility>
#include "UserMgr.h"
#include <QListWidget>
ApplyFriendItem::ApplyFriendItem(QWidget *parent) : QWidget(parent), ui(new Ui::ApplyFriendItem), _added(false) {
    ui->setupUi(this);

    ui->already_add_label->hide();

    connect(ui->add_btn, &QPushButton::clicked,  [this](){
        emit this->sig_auth_friend(_apply_info);
    });


}

ApplyFriendItem::~ApplyFriendItem() {
    delete ui;
}

void ApplyFriendItem::SetInfo(std::shared_ptr<ApplyInfo> apply_info)
{
    _apply_info = apply_info;
    // 加载图片
        UserMgr::SetLabelAvatar(ui->icon_lb, _apply_info->_icon);

    ui->user_name_label->setText(_apply_info->_name);
    ui->user_chat_label->setText(_apply_info->_desc);
}

void ApplyFriendItem::ShowAddBtn(bool bshow)
{
    if (bshow) {
        ui->add_btn->show();
        ui->already_add_label->hide();
        _added = false;
    }
    else {
        ui->add_btn->hide();
        ui->already_add_label->show();
        _added = true;
    }
}

int ApplyFriendItem::GetUid() {
    return _apply_info->_uid;
}