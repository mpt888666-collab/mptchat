//
// Created by mpt on 2026/9/21.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ChatGroupInfoShow.h" resolved

#include "chatgroupinfoshow.h"
#include "ui_ChatGroupInfoShow.h"


ChatGroupInfoShow::ChatGroupInfoShow(QWidget *parent) : QDialog(parent), ui(new Ui::ChatGroupInfoShow) {
    ui->setupUi(this);
}

ChatGroupInfoShow::~ChatGroupInfoShow() {
    delete ui;
}