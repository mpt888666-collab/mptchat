//
// Created by mpt on 2026/7/9.
//

#ifndef MPTCHAT_CHATDIALOG_H
#define MPTCHAT_CHATDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QJsonArray>
#include <QJsonObject>

#include "global.h"
class TextChatData;
struct ChatThreadInfo;
class TextChatMsg;
struct AuthRsp;
struct AuthInfo;
class AddFriendApply;
struct UserInfo;
class ChatThreadData;
class ChatUserWid;
class applygroupchat;
QT_BEGIN_NAMESPACE

namespace Ui {
    class ChatDialog;
}

QT_END_NAMESPACE

class ChatDialog : public QDialog {
    Q_OBJECT

public:
    explicit ChatDialog(QWidget *parent = nullptr);

    ~ChatDialog() override;

    void loadChatList();

private:
    Ui::ChatDialog *ui;

    ChatUIMode _state;
    ChatUIMode _mode;
    bool _b_loading;
    QMap<int, QListWidgetItem*>  _chat_thread_items;
    QMap<int, QListWidgetItem*>  _chat_id_items;
    QTimer *_timer;
    int _cur_chat_thread_id;
    std::shared_ptr<ChatThreadData> _cur_load_chat;
    std::shared_ptr<ChatThreadData> _next_load_chat;

    applygroupchat *_applygroup_chat;
    QMenu * _more_label_meum;
    void ShowSearch(bool);
    void handleGlobalMousePress(QMouseEvent *mouseEvent);
    void UpdateChatMsg(int friend_uid, QJsonArray contents);
    //void loadAddGroupChat();

    void RenderChatHistory(std::shared_ptr<ChatThreadData> thread_data);
    void fillGroupAvatars(ChatUserWid *chat_item, const QVector<int> &members);

   void addChatUserList();
    void SetSelectChatItem(int);
    void SetSelectChatPage(int);

    void load_chat_msg();

signals:
    void sig_show_add_group();

    void sig_user_logout();
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
private slots:
    void slot_loading_chat_user();

    void slot_side_chat();

    void slot_side_contact();

    void slot_side_config();

    void slot_apply_friend(std::shared_ptr<AddFriendApply>);

    void slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info);

    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);

    void slot_refresh_chat_user();

    void slot_text_chat_msg(std::vector<std::shared_ptr<TextChatData>> chat_msg);

    void slot_img_chat_msg(const QJsonObject &msg);

    void slot_chat_msg_rsp(std::vector<std::shared_ptr<TextChatData>> chat_msg);

    void slot_chat_item_clicked(QListWidgetItem *item);

    void slot_load_chat_thread(bool load_more, int last_thread_id,
        std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads);

    void slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info);

    void slot_con_item_clicked(QListWidgetItem *item);

    void slot_create_private_chat(int, int, int);

    void slot_load_chat_msg(int, int, bool, std::vector<std::shared_ptr<TextChatData>>);

    void slot_show_head_icon();

    void slot_reset_icon(QString);
    void slot_refresh_group_avatars();

    void slot_show_add_group();

    void slot_group_chat_item(QVector<int> members, int thread_id);

    void slot_notify_group_chat_item(QVector<int> members, int host_uid, int thread_id);

    void slot_more_select();

    void slot_logout();

    void slot_show_red_point_chat(int thread_id);



};


#endif //MPTCHAT_CHATDIALOG_H
