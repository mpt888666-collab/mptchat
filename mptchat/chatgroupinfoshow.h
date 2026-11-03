//
// Created by mpt on 2026/9/21.
//

#ifndef MPTCHAT_CHATGROUPINFOSHOW_H
#define MPTCHAT_CHATGROUPINFOSHOW_H

#include <QDialog>
#include <QVector>

class QLabel;
class QVBoxLayout;
class QWidget;

class ChatGroupInfoShow : public QDialog {
    Q_OBJECT

public:
    explicit ChatGroupInfoShow(QWidget *parent = nullptr);

    ~ChatGroupInfoShow() override;

    void SetGroupInfo(const QString &name, const QVector<int> &members, int owner_uid);

signals:
    void sig_clear_chat_requested();
    void sig_invite_members(const QVector<int> &members);
    void sig_kick_member(int uid);

private:
    void onInviteClicked();

    QLabel *_avatar_label{nullptr};
    QLabel *_name_label{nullptr};
    QLabel *_count_label{nullptr};
    QVBoxLayout *_member_layout{nullptr};
    QVector<int> _members;
    int _owner_uid{0};
};

#endif //MPTCHAT_CHATGROUPINFOSHOW_H
