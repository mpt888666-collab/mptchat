#ifndef MPTCHAT_CHATPAGE_H
#define MPTCHAT_CHATPAGE_H

#include <QWidget>
#include <QPixmap>
#include <QJsonArray>
#include <QMap>
#include <QList>
#include <memory>
#include <vector>

#include "PictureBubble.h"

class ChatThreadData;
class TextChatData;
class ChatDataBase;
QT_BEGIN_NAMESPACE
struct UserInfo;
namespace Ui { class ChatPage; }
QT_END_NAMESPACE

class ChatPage : public QWidget {
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage() override;
    void SetUserInfo(std::shared_ptr<UserInfo> info);

    // Append a received message from the other user
    void AppendOtherMessage(int sender_uid, const QJsonArray &contents);

    void setChatData(std::shared_ptr<ChatThreadData>);

    void ClearChatMessages();

    void AppendHistoryMessages(const std::vector<std::shared_ptr<TextChatData>>& msgs);

    void AppendPendingMessages(const QMap<QString, std::shared_ptr<ChatDataBase>>& pending);

    void AppendChatItems(const std::vector<std::shared_ptr<ChatDataBase>>& msgs);
    void AppendRemoteImage(const QString &image_name, int owner_uid);
    void AppendRemoteFile(const QString &file_name, int owner_uid);

    void ScrollToBottom();

    void SetTitleName(QString name);

    void setGroupUids(const QVector<int> &uids);

    // private chat: _user_info is set; group chat: _group_members_uid is not empty.
    // Both empty means there is no usable conversation, so nothing may be rendered or sent.
    bool hasActiveChat() const
    {
        return _user_info || !_group_members_uid.isEmpty();
    }

    bool isGroupChat() const
    {
        return !_group_members_uid.isEmpty();
    }

    void clearGroupUids()
    {
        _group_members_uid.clear();
    }

    void clearUserInfo() {
        _user_info = nullptr;
    }

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onSendClicked();
    void onReceiveClicked();
    void refreshAvatars();
    void onImgDownloaded(const QString &name, const QString &localPath);

private:
    QPixmap loadAvatarPixmap(const QString &iconFileName) const;
    void resolveSender(int send_uid, ChatRole role, QString &name, QPixmap &icon) const;
    void appendImageMessage(const QString &image_name, ChatRole role,
                            const QString &user_name, const QPixmap &user_icon,
                            int owner_uid);
    void appendFileMessage(const QString &file_name, ChatRole role,
                            const QString &user_name, const QPixmap &user_icon,
                            int owner_uid);

    Ui::ChatPage *ui;

    std::shared_ptr<UserInfo> _user_info;
    std::shared_ptr<ChatThreadData> _chat_data;
    QMap<QString, QList<PictureBubble*>> _pending_pictures;
    QVector<int> _group_members_uid;
};

#endif //MPTCHAT_CHATPAGE_H
