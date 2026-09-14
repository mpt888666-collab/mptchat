//
// Created by mpt on 2026/7/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ApplyFriendPage.h" resolved

#include "applyfriendpage.h"
#include "userdata.h"
#include <QPainter>

#include "applyfrienditem.h"
#include "authenfriend.h"
#include "ui_ApplyFriendPage.h"
#include "TCPMgr.h"
#include "usermgr.h"

ApplyFriendPage::ApplyFriendPage(QWidget *parent) : QWidget(parent), ui(new Ui::ApplyFriendPage) {
    ui->setupUi(this);
    loadApplyList();
    connect(TCPMgr::instance().get(), &TCPMgr::sig_auth_rsp, this, &ApplyFriendPage::slot_auth_rsp);

}

void ApplyFriendPage::AddApplyList() {
    auto apply_list = UserMgr::instance()->GetApplyList();
    for (const auto& apply : apply_list) {
        auto * apply_friend_item = new ApplyFriendItem();
        apply_friend_item->SetInfo(apply);
        auto * item = new QListWidgetItem();
        item->setSizeHint(apply_friend_item->sizeHint());
        ui->apply_friend_list->insertItem(0, item);
        ui->apply_friend_list->setItemWidget(item, apply_friend_item);
    }
}

ApplyFriendPage::~ApplyFriendPage() {
    delete ui;
}

void ApplyFriendPage::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ApplyFriendPage::AddNewApply(std::shared_ptr<AddFriendApply> apply) {
    qDebug() << " ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ApplyFriendPage::AddNewApply";
    auto apply_item = new ApplyFriendItem();
    auto apply_info = std::make_shared<ApplyInfo>(apply->_from_uid,
             apply->_name, apply->_desc,apply->_icon, apply->_name, apply->_sex, 0);

    apply_item->SetInfo(apply_info);
    connect(apply_item, &ApplyFriendItem::sig_auth_friend, this, [this, uid = apply->_from_uid, apply_info](std::shared_ptr<ApplyInfo> info) {
        auto* authFriend = new AuthenFriend(this);
        authFriend->setModal(true);
        authFriend->SetApplyInfo(apply_info);
        authFriend->show();
    });
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(apply_item->sizeHint());
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    ui->apply_friend_list->insertItem(0,item);
    ui->apply_friend_list->setItemWidget(item, apply_item);
    apply_item->ShowAddBtn(true);
    _unauth_items[apply->_from_uid] = apply_item;
}

void ApplyFriendPage::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp) {
    auto uid = auth_rsp->_uid;
    auto find_iter = _unauth_items.find(uid);
    if (find_iter == _unauth_items.end()) {
        return;
    }

    find_iter->second->ShowAddBtn(false);
}

void ApplyFriendPage::loadApplyList() {
    //添加好友申请
    auto apply_list = UserMgr::instance()->GetApplyList();
    for(auto &apply: apply_list){
        auto* apply_item = new ApplyFriendItem();
        apply_item->SetInfo(apply);
        QListWidgetItem* item = new QListWidgetItem;
        //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        item->setSizeHint(apply_item->sizeHint());
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        ui->apply_friend_list->insertItem(0,item);
        ui->apply_friend_list->setItemWidget(item, apply_item);
        if(apply->_status == 1){
            apply_item->ShowAddBtn(false);
        }else if (apply->_status == 0){
            apply->_status = 1;
            apply_item->ShowAddBtn(true);
            auto uid = apply_item->GetUid();
            _unauth_items[uid] = apply_item;
        }

        //收到审核好友信号
        connect(apply_item, &ApplyFriendItem::sig_auth_friend, [this](std::shared_ptr<ApplyInfo> apply_info) {
            auto* authFriend = new AuthenFriend(this);
            authFriend->setModal(true);
            authFriend->SetApplyInfo(apply_info);
            authFriend->show();
            });
    }
}

