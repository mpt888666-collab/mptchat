//
// Created by mpt on 2026/9/8.
//

// You may need to build the project (run Qt uic code generator) to get "ui_addgroupitem.h" resolved

#include "addgroupitem.h"

#include <QListWidgetItem>

#include "ui_addgroupitem.h"
#include "userdata.h"
#include "UserMgr.h"

addgroupitem::addgroupitem(QWidget *parent) : ListItemBase(parent), ui(new Ui::addgroupitem) {
    ui->setupUi(this);
}

addgroupitem::~addgroupitem() {
    delete ui;
}

void addgroupitem::SetUserInfo(std::shared_ptr<UserInfo> f) {
    _user_info = f;
    _uid = f->_uid;
    _name = f->_name;
    _icon = f->_icon;

    ui->name_label->setText(_name);

    UserMgr::SetLabelAvatar(ui->icon1_label, _icon);
}

void addgroupitem::set_pd_label() {
    _selected = !_selected;
    ui->pd_label->setChecked(_selected);
}

void addgroupitem::CloseLabel() {
    ui->pd_label->hide();
}


