//
// Created by mpt on 2026/7/20.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FindSuccessDlg.h" resolved

#include "findsuccessdlg.h"
#include "ui_FindSuccessDlg.h"
#include "userdata.h"
#include "UserMgr.h"
FindSuccessDlg::FindSuccessDlg(QWidget *parent) : QDialog(parent), ui(new Ui::FindSuccessDlg), _parent(parent){
    ui->setupUi(this);
    setWindowTitle("添加");
    // 隐藏对话框标题栏
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    this->setObjectName("FindSuccessDlg");
    connect(ui->add_friend_btn, &QPushButton::clicked,this, [&]() {
        this->hide();
        auto applyFriend = new ApplyFriend(_parent);
        applyFriend->SetSearchInfo(_si);
        applyFriend->setModal(true);
        applyFriend->show();
    });
    this->setModal(true);
}

FindSuccessDlg::~FindSuccessDlg() {
    delete ui;
}

void FindSuccessDlg::SetSearchInfo(std::shared_ptr<SearchInfo> si) {
    _si = si;
    ui->name_label->setText(si->_name);
    UserMgr::SetLabelAvatar(ui->head_label, si->_icon);
}



