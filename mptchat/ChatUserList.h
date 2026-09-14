//
// Created by mpt on 2026/7/12.
//

#ifndef MPTCHAT_CHATUSERLIST_H
#define MPTCHAT_CHATUSERLIST_H
#include <QListWidget>
#include "global.h"
class ChatUserList : public QListWidget{
    Q_OBJECT
public:
    explicit ChatUserList(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

signals:
    void sig_loading_chat_user();
};


#endif //MPTCHAT_CHATUSERLIST_H