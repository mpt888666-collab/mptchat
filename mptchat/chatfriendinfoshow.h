//
// Created by mpt on 2026/9/21.
//

#ifndef MPTCHAT_CHATFRIENDINFOSHOW_H
#define MPTCHAT_CHATFRIENDINFOSHOW_H

#include <QDialog>


QT_BEGIN_NAMESPACE

namespace Ui {
    class ChatFriendInfoShow;
}

QT_END_NAMESPACE

class ChatFriendInfoShow : public QDialog {
    Q_OBJECT

public:
    explicit ChatFriendInfoShow(QWidget *parent = nullptr);

    ~ChatFriendInfoShow() override;

    void SetPrivateChatInfo(const QString& icon);

private:
    Ui::ChatFriendInfoShow *ui;

    QString _icon;
};


#endif //MPTCHAT_CHATFRIENDINFOSHOW_H