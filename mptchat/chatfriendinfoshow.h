//
// Created by mpt on 2026/9/21.
//

#ifndef MPTCHAT_CHATFRIENDINFOSHOW_H
#define MPTCHAT_CHATFRIENDINFOSHOW_H

#include <QDialog>

class QLabel;

class ChatFriendInfoShow : public QDialog {
    Q_OBJECT

public:
    explicit ChatFriendInfoShow(QWidget *parent = nullptr);

    ~ChatFriendInfoShow() override;

    void SetPrivateChatInfo(const QString &name, const QString &icon, int uid);

signals:
    void sig_clear_chat_requested();

private:
    QLabel *_avatar_label{nullptr};
    QLabel *_name_label{nullptr};
    QLabel *_uid_label{nullptr};
};

#endif //MPTCHAT_CHATFRIENDINFOSHOW_H
