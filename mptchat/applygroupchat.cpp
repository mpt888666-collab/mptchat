//
// Created by mpt on 2026/9/8.
//

// You may need to build the project (run Qt uic code generator) to get "ui_applygroupchat.h" resolved

#include "applygroupchat.h"

#include "addgroupitem.h"
#include "ui_applygroupchat.h"
#include <QJsonObject>

#include "TCPMgr.h"
#include "userdata.h"
#include "usermgr.h"

applygroupchat::applygroupchat(QWidget *parent) : QDialog(parent), ui(new Ui::applygroupchat) {
    ui->setupUi(this);

    auto *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":icons/res/search.ico"));
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索好友"));

    auto clearAction = new QAction(ui->search_edit);
    clearAction->setCheckable(true);
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);

    connect(ui->search_edit, &QLineEdit::textChanged, this, [this, clearAction](QString text) {
        if (text.isEmpty()) {
            clearAction->setIcon(QIcon(":icons/res/empty.ico"));
        }else {
            clearAction->setIcon(QIcon(":icons/res/clear_visible.ico"));
        }
    });

    connect(ui->cancel_btn, &QPushButton::clicked, this, &applygroupchat::slot_cancel_btn);
    connect(ui->confirm_btn, &QPushButton::clicked, this, &applygroupchat::slot_send_add_group_request);

    connect(ui->friend_list,&QListWidget::itemPressed,this,&applygroupchat::slot_add_group);

    ui->group_frined_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->group_frined_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

}

applygroupchat::~applygroupchat() {
    clearItems();
    delete ui;
}

void applygroupchat::addItem(addgroupitem* friend_item) {
    if (!friend_item) {
        return;
    }

    const int uid = friend_item->GetUid();
    if (_friend_list_map.contains(uid)) {
        return;
    }

    _friend_list_map[uid] = friend_item;

    auto *item = new QListWidgetItem;
    ui->friend_list->insertItem(0, item);
    ui->friend_list->setItemWidget(item, friend_item);
    const QSize widget_hint = friend_item->sizeHint();
    const int row_width = qMax(ui->friend_list->viewport()->width(), widget_hint.width());
    item->setSizeHint(QSize(row_width, widget_hint.height()));
}

void applygroupchat::clearItems() {
    ui->friend_list->clear();
    _friend_list_map.clear();
}

void applygroupchat::clearItemsGroup() {
    ui->group_frined_list->clear();
    _group_friend_list_map.clear();
}

void applygroupchat::slot_cancel_btn() {
    clearItemsGroup();
    this->hide();
}

void applygroupchat::slot_add_group(QListWidgetItem *item) {
    QWidget * widget = ui->friend_list->itemWidget(item);
    if (widget) {
        auto friend_item = qobject_cast<addgroupitem*>(widget);
        if (friend_item) {
            friend_item->set_pd_label();
            if (friend_item->GetSelect()) {
                auto *qitem = new QListWidgetItem;
                qitem->setSizeHint(friend_item->sizeHint());

                auto new_friend_item = new addgroupitem();
                new_friend_item->CloseLabel();
                auto user_info = friend_item->GetUserInfo();
                new_friend_item->SetUserInfo(user_info);

                ui->group_frined_list->insertItem(0, qitem);
                ui->group_frined_list->setItemWidget(qitem, new_friend_item);
                auto iter = _group_friend_list_map.find(friend_item->GetUid());
                if (iter == _group_friend_list_map.end()) {
                    _group_friend_list_map[friend_item->GetUid()] = new_friend_item;
                }
            }else {
                auto iter = _group_friend_list_map.find(friend_item->GetUid());
                if (iter != _group_friend_list_map.end()) {
                    int uid = iter.value()->GetUid();
                    _group_friend_list_map.erase(iter);
                }

                for(int i = 0; i < ui->group_frined_list->count(); i++)
                {
                    auto right_item = ui->group_frined_list->item(i);
                    auto w = ui->group_frined_list->itemWidget(right_item);
                    auto right_friend = qobject_cast<addgroupitem*>(w);
                    if(right_friend && right_friend->GetUid() == friend_item->GetUid())
                    {
                        delete ui->group_frined_list->takeItem(i);
                        break;
                    }
                }
            }
        }
    }
}

void applygroupchat::slot_send_add_group_request() {
    if (!_group_friend_list_map.empty()) {
        QJsonObject jsonObj;
        QJsonArray jsonArray;
        jsonObj["host_uid"] = UserMgr::instance()->GetUid();
        for (const auto &iter : _group_friend_list_map) {
            jsonArray.append(iter->GetUid());
        }
        jsonObj["members"] = jsonArray;
        QJsonDocument jsonDoc(jsonObj);
        QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);
        emit TCPMgr::instance()->sig_send_data(ID_CREATE_GROUP_CHAT_REQ, jsonData);
        clearItemsGroup();
        this->hide();
    }
}

