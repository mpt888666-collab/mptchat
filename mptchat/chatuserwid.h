//
// Created by mpt on 2026/7/12.
//

#ifndef MPTCHAT_CHATUSERWID_H
#define MPTCHAT_CHATUSERWID_H

#include <complex.h>
#include <QWidget>
#include "ListItemBase.h"
#include "userdata.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class ChatUserWid;
}

QT_END_NAMESPACE

class ChatUserWid : public ListItemBase {
    Q_OBJECT

public:
    explicit ChatUserWid(QWidget *parent = nullptr);
    explicit ChatUserWid(ChatFormType type, QWidget *parent = nullptr);

    ~ChatUserWid() override;

    QSize sizeHint() const override {
        return QSize(250, 70);
    }

    void SetInfo(QString name, QString head, QString msg);

    void SetInfo(std::shared_ptr<UserInfo> user_info);

    void SetInfo(int uid, QString name, QString head, QString msg);

    void updateLastMsg(QJsonArray msg);

    void SetLastMsg(const QString& msg);

    void SetChatData(std::shared_ptr<ChatThreadData> chat_data);

    void SetSelected(bool selected);

    int GetUid() const { return _uid; }

    void SetAvatarList(QVector<QPixmap> &ava);

    void SetThreadId(int thread_id);

    int GetThreadId() const;

    void SetName(QString name);

    void SetGroupHead(int spacing, int maxShowCount, QVector<QPixmap> &avatarList);

    void SetGroupUids(const QVector<int> &uids);

    void ShowRedPoint(bool);

    QString GetGroupName()const {return _name;}

    [[nodiscard]] ChatFormType GetChatType() const {return _chat_type;}

    std::shared_ptr<ChatThreadData> GetChatData();

    QVector<int> GetGroupUids();
private:
    Ui::ChatUserWid *ui;

    int _uid = 0;
    int _thread_id{-1};
    QString _name;
    QString _head;
    QString _msg;

    std::shared_ptr<ChatThreadData> _chat_data;

    ChatFormType _chat_type;

    QVector<int> _group_friend_uid;
    QVector<QPixmap> _avatarList;
};


#endif //MPTCHAT_CHATUSERWID_H
