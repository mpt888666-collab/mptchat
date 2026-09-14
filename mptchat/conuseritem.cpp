//
// Created by mpt on 2026/7/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ConUserItem.h" resolved

#include "conuseritem.h"
#include "ui_ConUserItem.h"
#include "UserMgr.h"


ConUserItem::ConUserItem(QWidget *parent) : ListItemBase(parent), ui(new Ui::ConUserItem) {
    ui->setupUi(this);
    SetItemType(ListItemType::CONTACT_USER_ITEM);
    ui->red_point->raise();
    ShowRedPoint(false);
}

ConUserItem::~ConUserItem() {
    delete ui;
}

QSize ConUserItem::sizeHint() const {
    return {250, 70};
}

void ConUserItem::SetInfo(int uid, const QString& name, const QString& icon) {
    _info = std::make_shared<UserInfo>(uid, name, icon);
    UserMgr::SetLabelAvatar(ui->icon_label, _info->_icon);

    ui->user_name_label->setText(_info->_name);
}

void ConUserItem::SetInfo(std::shared_ptr<AuthInfo> auth_info)
{
    _info = std::make_shared<UserInfo>(auth_info);
    // 加载图片
        UserMgr::SetLabelAvatar(ui->icon_label, _info->_icon);

    ui->user_name_label->setText(_info->_name);
}

void ConUserItem::ShowRedPoint(bool show)
{
    if(show){
        ui->red_point->show();
    }else{
        ui->red_point->hide();
    }

}