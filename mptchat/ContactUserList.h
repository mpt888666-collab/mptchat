//
// Created by mpt on 2026/7/25.
//

#ifndef MPTCHAT_CONTACTUSERLIST_H
#define MPTCHAT_CONTACTUSERLIST_H
#include <QListWidget>
#include <QWidget>

#include "conuseritem.h"

class ContactUserList : public QListWidget{
    Q_OBJECT
public:
    explicit ContactUserList(QWidget *parent = nullptr);
    void ShowRedPoint(bool bshow = true);

    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void slot_item_clicked(QListWidgetItem *item);
    void slot_refresh_con();
    void slot_add_auth_firend(std::shared_ptr<AuthInfo> auth_info);
private:

    void addContactUserList();
signals:
    void sig_switch_apply_friend_page();
    void sig_switch_friend_info_page();
    void sig_loading_contact_user();


private:
    ConUserItem* _add_friend_item{};
    QListWidgetItem * _groupitem{};
};


#endif //MPTCHAT_CONTACTUSERLIST_H