//
// Created by mpt on 2026/7/14.
//

#include "SearchList.h"
#include <QListWidgetItem>
#include <QListWidget>
#include "adduseritem.h"
#include "customizeedit.h"
#include "TCPMgr.h"
#include "FindSuccessDlg.h"
SearchList::SearchList(QWidget *parent) : QListWidget(parent),_find_dlg(nullptr), _search_edit(nullptr), _send_pending(false){
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->viewport()->installEventFilter(this);
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_user_search, this, &SearchList::slot_user_search);
    addTipItem();
}

void SearchList::addTipItem() {
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item_tmp->setSizeHint(QSize(250,10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);


    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    item->setData(Qt::UserRole, static_cast<int>(ListItemType::ADD_USER_TIP_ITEM));
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, add_user_item);
}

void SearchList::slot_item_clicked(QListWidgetItem *item) {

    QWidget *widget = this->itemWidget(item);
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = currentItem()->data(Qt::UserRole).toInt();
    if(itemType == ListItemType::INVALID_ITEM){
        qDebug()<< "slot invalid item clicked ";
        return;
    }
    if(itemType == ListItemType::ADD_USER_TIP_ITEM){
        if (_send_pending) return;
        if (!_search_edit) return;

        auto search_edit = dynamic_cast<CustomizeEdit*>(_search_edit);
        auto uid_string = search_edit->text();
        QJsonObject jsonObj;
        jsonObj.insert("uid", uid_string);

        QJsonDocument jsonDoc(jsonObj);
        QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);
        emit TCPMgr::instance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ,
                                                  jsonData);

        return;
    }
    CloseFindDlg();
}

void SearchList::SetSearchEdit(QWidget *edit) {
    _search_edit = edit;
}

void SearchList::CloseFindDlg() {
    if(_find_dlg) {
        _find_dlg->hide();
        _find_dlg = nullptr;
    }
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si) {
    if(si == nullptr){
        // _find_dlg = std::make_shared<FindFailDlg>(this);
    }else{
        //此处分两种情况，一种是搜多到已经是自己的朋友了，一种是未添加好友
        //查找是否已经是好友 todo...
        _find_dlg = std::make_shared<FindSuccessDlg>(this);
        std::dynamic_pointer_cast<FindSuccessDlg>(_find_dlg)->SetSearchInfo(si);
        _find_dlg->show();
    }

}




