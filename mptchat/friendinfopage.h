//
// Created by mpt on 2026/8/15.
//

#ifndef MPTCHAT_FRIENDINFOPAGE_H
#define MPTCHAT_FRIENDINFOPAGE_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui {
    class FriendInfoPage;
}

QT_END_NAMESPACE
struct UserInfo;
class FriendInfoPage : public QWidget {
    Q_OBJECT

public:
    explicit FriendInfoPage(QWidget *parent = nullptr);

    ~FriendInfoPage() override;

    void setInfo(std::shared_ptr<UserInfo>);

private slots:
    void on_msg_chat_clicked();

private:
    void refreshAvatar();

    std::shared_ptr<UserInfo> _user_info;
    Ui::FriendInfoPage *ui;

signals:
    void sig_jump_chat_item(std::shared_ptr<UserInfo> si);
};


#endif //MPTCHAT_FRIENDINFOPAGE_H
