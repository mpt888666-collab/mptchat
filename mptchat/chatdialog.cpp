//
// Created by mpt on 2026/7/9.
//

#include "chatdialog.h"
#include "ui_ChatDialog.h"
#include <QAction>
#include <QDir>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <qmenu.h>

#include "global.h"
#include "chatuserwid.h"
#include <QRandomGenerator>
#include <QStandardPaths>
#include "UserMgr.h"
#include <QTimer>
#include "applygroupchat.h"
#include "addgroupitem.h"
#include "friendinfopage.h"
#include "TCPFileMgr.h"
#include "ui_addgroupitem.h"
#include <QListWidgetItem>
void ChatDialog::addChatUserList() {
    auto friend_list = UserMgr::instance()->GetFriendList();
    for (const auto& fr : friend_list) {
        auto *chat_user_item = new ChatUserWid();

        chat_user_item->SetInfo(fr->_uid, fr->_name, fr->_icon, "");

        auto *chat_item = new QListWidgetItem();
        chat_item->setSizeHint(chat_user_item->sizeHint());

        ui->chat_user_list->insertItem(0, chat_item);
        ui->chat_user_list->setItemWidget(chat_item, chat_user_item);
        _chat_id_items.insert(fr->_uid, chat_item);

        auto thread_id = UserMgr::instance()->GetChatThreadByUid(fr->_uid);
        if (thread_id == -1) {
            qDebug() << "UserMgr::GetChatThreadByUid error";
            return;
        }
        _chat_thread_items.insert(UserMgr::instance()->GetChatThreadByUid(fr->_uid), chat_item);
        chat_user_item->SetChatData(std::make_shared<ChatThreadData>(fr->_uid, thread_id, 0));
    }
}


ChatDialog::ChatDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ChatDialog), _state(ChatMode), _mode(ChatMode), _b_loading(false), _cur_chat_thread_id(-1), _more_label_meum(nullptr) {
    ui->setupUi(this);
    ShowSearch(false);
    ui->search_list->SetSearchEdit(ui->search_edit);
    ui->chat_page->hide();
    ui->friend_info_page->hide();
    ui->user_info_page->hide();
    //addChatUserList();
    _applygroup_chat = new applygroupchat(this);
    _more_label_meum = new QMenu(this);
    QAction* actLogout = _more_label_meum->addAction("退出登录");
    connect(actLogout, &QAction::triggered, this, &ChatDialog::slot_logout);
    ui->chat_label->AddRedPoint();

    installEventFilter(this);
    ui->more_label->setProperty("state", "normal");
    ui->more_label->SetState("normal", "", "", "selected", "", "");
    ui->chat_label->setProperty("state", "selected");
    ui->contact_label->setProperty("state", "normal");
    ui->chat_label->SetState("normal", "", "", "selected", "", "");
    ui->contact_label->SetState("normal", "", "", "selected", "", "");
    ui->config_label->setProperty("state", "normal");
    ui->config_label->SetState("normal", "", "", "selected", "", "");

    auto *searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":icons/res/search.ico"));
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    auto clearAction = new QAction(ui->search_edit);
    clearAction->setCheckable(true);
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);

    connect(ui->search_edit, &QLineEdit::textChanged, this, [this, clearAction](QString text) {
        if (text.isEmpty()) {
            clearAction->setIcon(QIcon(":icons/res/empty.ico"));
            ShowSearch(false);
        }else {
            clearAction->setIcon(QIcon(":icons/res/clear_visible.ico"));
            ShowSearch(true);
        }
    });

    connect(ui->friend_info_page, &FriendInfoPage::sig_jump_chat_item, this,
        &ChatDialog::slot_jump_chat_item_from_infopage);

    connect(clearAction, &QAction::triggered, this, [this, clearAction](bool checked) {
        if (checked) {
            ui->search_edit->clear();
            ShowSearch(false);
        }
    });

     _timer = new QTimer(this);
     connect(_timer, &QTimer::timeout, this, [this](){
             auto user_info = UserMgr::instance()->GetUserInfo();
             if (!user_info) return;
             QJsonObject textObj;
             textObj["fromuid"] = user_info->_uid;
             QJsonDocument doc(textObj);
             QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
             emit TCPMgr::instance()->sig_send_data(ReqId::ID_HEART_BEAT_REQ, jsonData);
     });

    _timer->start(10000);

    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user, this, &ChatDialog::slot_loading_chat_user);
    connect(ui->chat_label, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->contact_label, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);
    connect(ui->config_label, &StateWidget::clicked, this, &ChatDialog::slot_side_config);
    connect(ui->add_btn, &QPushButton::clicked, this, &ChatDialog::slot_show_add_group);

    connect(ui->con_user_list, &ContactUserList::sig_switch_apply_friend_page, this, [this](QListWidgetItem *item) {
        ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
        auto *widget = ui->con_user_list->itemWidget(item);
        if (!widget) return;
        auto con_item = qobject_cast<ConUserItem*>(widget);
        con_item->ShowRedPoint(false);
    });

    // Click on a chat user to open their conversation
    connect(ui->chat_user_list, &QListWidget::itemClicked, this, &ChatDialog::slot_chat_item_clicked);
    connect(ui->con_user_list, &QListWidget::itemClicked, this, &ChatDialog::slot_con_item_clicked);
    connect(ui->more_label, &StateWidget::clicked, this, &ChatDialog::slot_more_select);

    connect(ui->chat_user_list, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *previous) {
                if (previous) {
                    if (auto *wid = qobject_cast<ChatUserWid *>(
                            ui->chat_user_list->itemWidget(previous))) {
                        wid->SetSelected(false);
                    }
                }
                if (current) {
                    if (auto *wid = qobject_cast<ChatUserWid *>(
                            ui->chat_user_list->itemWidget(current))) {
                        wid->SetSelected(true);
                    }
                }
            });

    connect(TCPMgr::instance().get(), &TCPMgr::sig_friend_apply, this, &ChatDialog::slot_apply_friend);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_auth_rsp, this, &ChatDialog::slot_auth_rsp);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_add_auth_friend, this, &ChatDialog::slot_add_auth_friend);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_switch_chatdlg, this, &ChatDialog::slot_refresh_chat_user);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_text_chat_msg, this, &ChatDialog::slot_text_chat_msg);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_img_chat_msg, this, &ChatDialog::slot_img_chat_msg);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_chat_msg_rsp, this, &ChatDialog::slot_chat_msg_rsp);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_load_chat_thread,this, &ChatDialog::slot_load_chat_thread);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_create_private_chat,this, &ChatDialog::slot_create_private_chat);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_load_chat_msg, this, &ChatDialog::slot_load_chat_msg);
    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_chatDialog_head_icon, this, &ChatDialog::slot_show_head_icon);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_add_group_chat_item, this, &ChatDialog::slot_group_chat_item);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_notify_add_group_chat_item, this, &ChatDialog::slot_notify_group_chat_item);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_show_red_point, this, &ChatDialog::slot_show_red_point_chat);

    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_reset_label_icon, this, &ChatDialog::slot_reset_icon);

    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_avatar_downloaded, this, &ChatDialog::slot_refresh_group_avatars);

}

ChatDialog::~ChatDialog() {
    _timer->stop();
    delete ui;
}

void ChatDialog::ShowSearch(bool bsearch) {
    if (bsearch) {
        ui->search_list->show();
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        _mode = ChatUIMode::SearchMode;
    }else if (_state == ChatUIMode::ChatMode) {
        ui->chat_user_list->show();
        ui->con_user_list->hide();
        ui->search_list->hide();
        _mode = ChatUIMode::ChatMode;
    }else if(_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        _mode = ChatUIMode::ContactMode;
    }
}

void ChatDialog::slot_loading_chat_user(){
    if(_b_loading){
        return;
    }

    _b_loading = true;
    qDebug() << "add new data to list.....";
    //addChatUserList();

    _b_loading = false;
}

void ChatDialog::slot_side_chat() {
    ui->chat_label->ShowRedPoint(false);
    ui->contact_label->ClearState();
    ui->config_label->ClearState();

    ui->con_user_list->clearSelection();
    ui->con_user_list->setCurrentItem(nullptr);
    ui->chat_user_list->clearSelection();
    ui->chat_user_list->setCurrentItem(nullptr);
    ui->con_user_list->setCurrentItem(nullptr);
    ui->friend_info_page->hide();
    ui->friend_apply_page->hide();
    ui->user_info_page->hide();

    // ui->stackedWidget->setCurrentWidget(ui->chat_page);
    //ui->chat_page->hide();
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);
    SetSelectChatItem(_cur_chat_thread_id);

    if (_cur_chat_thread_id != -1) {
        auto thread_data = UserMgr::instance()->GetChatThreadDataByThreadId(_cur_chat_thread_id);
        auto iter = _chat_thread_items.find(_cur_chat_thread_id);
        if (iter == _chat_thread_items.end()) {
            qDebug() << "thread id is null";
            return;
        }
        auto *item = _chat_thread_items[_cur_chat_thread_id];
        QWidget* widget = ui->chat_user_list->itemWidget(item);
        auto chat_item = qobject_cast<ChatUserWid*>(widget);
        if (thread_data && chat_item) {
            if (chat_item->GetChatType() == ChatFormType::PRIVATE) {
                auto friend_info = UserMgr::instance()->GetUserInfoById(thread_data->GetOtherId());
                if (friend_info) {
                    ui->chat_page->SetUserInfo(friend_info);
                    ui->chat_page->clearGroupUids();
                }
                ui->chat_page->setChatData(thread_data);
                RenderChatHistory(thread_data);
            }else {
                auto group_uids = thread_data->GetGroupMembers();
                if (group_uids.isEmpty()) {
                    group_uids = chat_item->GetGroupUids();
                }
                if (group_uids.isEmpty()) {
                    group_uids.push_back(UserMgr::instance()->GetUid());
                }
                ui->chat_page->setGroupUids(group_uids);
                ui->chat_page->SetTitleName(chat_item->GetGroupName());
                ui->chat_page->setChatData(thread_data);
                RenderChatHistory(thread_data);
            }
        }
    }
}

void ChatDialog::slot_side_contact() {
    ui->chat_label->ClearState();
    ui->config_label->ClearState();

    ui->chat_user_list->clearSelection();
    ui->chat_user_list->setCurrentItem(nullptr);
    ui->con_user_list->clearSelection();
    ui->con_user_list->setCurrentItem(nullptr);
    ui->chat_user_list->setCurrentItem(nullptr);
    // ui->chat_page->hide();

    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
    _state = ChatUIMode::ContactMode;
    ui->contact_label->ShowRedPoint(false);
    ShowSearch(false);
}

void ChatDialog::slot_side_config() {
    ui->chat_label->ClearState();
    ui->contact_label->ClearState();

    ui->con_user_list->clearSelection();
    ui->chat_user_list->clearSelection();
    ui->con_user_list->setCurrentItem(nullptr);
    ui->chat_user_list->setCurrentItem(nullptr);

    ui->stackedWidget->setCurrentWidget(ui->user_info_page);
    ui->user_info_page->show();
    ShowSearch(false);
}

bool ChatDialog::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        handleGlobalMousePress(mouseEvent);
    }
    return QDialog::eventFilter(watched, event);
}

void ChatDialog::handleGlobalMousePress(QMouseEvent *mouseEvent) {
    if (_mode != ChatUIMode::SearchMode) {
        return;
    }

    QPoint globalPos = ui->search_list->viewport()->mapToGlobal(mouseEvent->globalPos());
    if (!ui->search_list->rect().contains(globalPos)) {
        ui->search_edit->clear();
        ShowSearch(false);
    }
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply) {
    qDebug() << "receive apply friend slot, applyuid is " << apply->_from_uid << " name is "
        << apply->_name << " desc is " << apply->_desc;

    bool b_already = UserMgr::instance()->AlreadyApply(apply->_from_uid);
    if(b_already){
        return;
    }
    UserMgr::instance()->AddApplyList(std::make_shared<ApplyInfo>(apply));
    ui->contact_label->ShowRedPoint(true);
    ui->con_user_list->ShowRedPoint(true);
    ui->friend_apply_page->AddNewApply(apply);
}

void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info) {
    qDebug() << "receive slot_add_auth__friend uid is " << auth_info->_uid
       << " name is " << auth_info->_name << " nick is " << auth_info->_nick;

    UserMgr::instance()->AddFriend(auth_info);

    auto* chat_user_item = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_item->SetInfo(user_info->_uid, user_info->_name, user_info->_icon, "");

    auto* item = new QListWidgetItem();
    item->setSizeHint(chat_user_item->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_item);
    //_chat_thread_items.insert(UserMgr::instance()->GetChatThreadByUid(auth_info->_uid), item);
    _chat_id_items.insert(auth_info->_uid, item);
}

void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    qDebug() << "receive slot_auth_rsp uid is " << auth_rsp->_uid
        << " name is " << auth_rsp->_name << " nick is " << auth_rsp->_nick;

    UserMgr::instance()->AddFriend(auth_rsp);

    auto* chat_user_wid = new ChatUserWid();
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    chat_user_wid->SetInfo(user_info->_uid, user_info->_name, user_info->_icon, "");
    QListWidgetItem* item = new QListWidgetItem;
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    //_chat_thread_items.insert(auth_rsp->_uid, item);
    _chat_id_items.insert(auth_rsp->_uid, item);
}

void ChatDialog::slot_refresh_chat_user() {
    ui->chat_user_list->clear();
    _chat_thread_items.clear();
    _chat_id_items.clear();
    addChatUserList();
    ui->friend_apply_page->loadApplyList();
}

void ChatDialog::slot_chat_item_clicked(QListWidgetItem *item) {
    QWidget *widget = ui->chat_user_list->itemWidget(item);
    auto *chat_wid = qobject_cast<ChatUserWid*>(widget);
    chat_wid->ShowRedPoint(false);
    if (!chat_wid) return;
    if (chat_wid->GetChatType() == ChatFormType::PRIVATE) {
        int friend_uid = chat_wid->GetUid();
        auto friend_info = UserMgr::instance()->GetUserInfoById(friend_uid);
        auto thread_id = UserMgr::instance()->GetChatThreadByUid(friend_uid);
        ui->chat_page->ScrollToBottom();
        if (thread_id != -1 && _state == ChatUIMode::ChatMode &&
            _cur_chat_thread_id == thread_id &&
            ui->stackedWidget->currentWidget() == ui->chat_page) {
            auto chat_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
            if (chat_data) {
                ui->chat_page->setChatData(chat_data);
                ui->chat_page->ScrollToBottom();
                return;
            }
        }
        if (thread_id == -1) {
            if (!friend_info) return;
            QJsonObject jsonObj;
            jsonObj["uid"] = UserMgr::instance()->GetUid();
            jsonObj["other_id"] = friend_uid;
            QJsonDocument jsonDoc(jsonObj);
            QByteArray data;
            data = jsonDoc.toJson(QJsonDocument::Compact);
            emit TCPMgr::instance()->sig_send_data(ID_CREATE_PRIVATE_CHAT_REQ, data);
            return;
        }else {
            auto iter = _chat_thread_items.find(thread_id);
            if (iter == _chat_thread_items.end()) {
                _chat_thread_items.insert(thread_id, item);
            }
            auto chat_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
            if (!chat_data) {
                chat_data = std::make_shared<ChatThreadData>(friend_uid, thread_id, 0);
                UserMgr::instance()->AddChatThreadData(chat_data);
            }
            chat_wid->SetChatData(chat_data);
        }
        if (!friend_info) return;

        // Set the chat page to this friend (so onSendClicked knows the touid)
        ui->chat_page->SetUserInfo(friend_info);
        ui->chat_page->clearGroupUids();
        ui->chat_page->show();

        // Switch to chat page
        ui->stackedWidget->setCurrentWidget(ui->chat_page);
        _state = ChatUIMode::ChatMode;
        ShowSearch(false);
        _cur_chat_thread_id = UserMgr::instance()->GetChatThreadByUid(friend_info->_uid);

        auto chat_data = chat_wid->GetChatData();
        ui->chat_page->setChatData(chat_data);
        RenderChatHistory(chat_data);
    }else {
        auto thread_id = chat_wid->GetThreadId();
        qDebug() << thread_id << " chat_dialog::slot_chat_item_clicked group";
        if (thread_id == -1) {
            qDebug() << "chat wid->getThreadId() -1";
            return;
        }
        auto iter = _chat_thread_items.find(thread_id);
        if (iter == _chat_thread_items.end()) {
            qDebug() << "chat wid->getThreadId() -1 nullptr";
            return;
        }
        ui->chat_page->ScrollToBottom();
        if (_state == ChatUIMode::ChatMode &&
            _cur_chat_thread_id == thread_id &&
            ui->stackedWidget->currentWidget() == ui->chat_page) {
            auto chat_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
            if (chat_data) {
                ui->chat_page->setChatData(chat_data);
                ui->chat_page->ScrollToBottom();
                return;
            }
        }
        auto chat_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
        if (!chat_data) {
            chat_data = std::make_shared<ChatThreadData>(0, thread_id, 0);
            UserMgr::instance()->AddChatThreadData(chat_data);
        }
        chat_wid->SetChatData(chat_data);
        auto group_uids = chat_data->GetGroupMembers();
        if (group_uids.isEmpty()) {
            group_uids = chat_wid->GetGroupUids();
        }
        if (group_uids.isEmpty()) {
            group_uids.push_back(UserMgr::instance()->GetUid());
        }
        ui->chat_page->setGroupUids(group_uids);
        ui->chat_page->SetTitleName(chat_wid->GetGroupName());
        ui->chat_page->show();
        ui->stackedWidget->setCurrentWidget(ui->chat_page);
        _state = ChatUIMode::ChatMode;
        ShowSearch(false);
        _cur_chat_thread_id = thread_id;
        auto chat_thread_data = chat_wid->GetChatData();
        ui->chat_page->setChatData(chat_thread_data);
        RenderChatHistory(chat_thread_data);

    }
}
void ChatDialog::RenderChatHistory(std::shared_ptr<ChatThreadData> thread_data) {
    if (!thread_data) return;

    std::vector<std::shared_ptr<ChatDataBase>> items;
    const auto& msg_order = thread_data->GetMsgOrderRef();
    items.reserve(static_cast<size_t>(msg_order.size()));
    for (const auto& entry : msg_order) {
        if (entry) {
            items.push_back(entry);
        }
    }

    ui->chat_page->ClearChatMessages();
    ui->chat_page->AppendChatItems(items);
    ui->chat_page->ScrollToBottom();
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
}

void ChatDialog::UpdateChatMsg(int friend_uid, QJsonArray contents) {
    ui->chat_page->AppendOtherMessage(friend_uid, contents);

    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    _state = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void ChatDialog::slot_text_chat_msg(
    std::vector<std::shared_ptr<TextChatData>> chat_msgs) {

    if (chat_msgs.empty()) {
        return;
    }

    // Messages are grouped by (thread id, sender id). A group message must only update the
    // existing group conversation item, it must never create a private conversation item.
    QMap<QPair<int, int>, QJsonArray> grouped_msgs;
    QMap<QPair<int, int>, bool> group_flags;

    for (const auto &msg : chat_msgs) {
        if (!msg) {
            continue;
        }

        const QPair<int, int> key(msg->GetThreadId(), msg->GetSendUid());

        QJsonObject obj;
        obj["message_id"] = msg->GetMsgId();
        obj["content"] = msg->GetMsgContent();
        obj["status"] = msg->GetStatus();
        obj["owner_uid"] = msg->GetSendUid();

        grouped_msgs[key].append(obj);
        group_flags[key] = (msg->GetFormType() == ChatFormType::GROUP);
    }

    for (auto it = grouped_msgs.constBegin(); it != grouped_msgs.constEnd(); ++it) {
        const int thread_id = it.key().first;
        const int sender_uid = it.key().second;
        const QJsonArray &contents = it.value();
        const bool is_group = group_flags.value(it.key(), false);

        std::shared_ptr<ChatThreadData> thread_data;
        if (thread_id > 0) {
            thread_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
        }

        if (thread_data) {
            for (const auto &msg : chat_msgs) {
                if (!msg) continue;
                if (msg->GetThreadId() != thread_id || msg->GetSendUid() != sender_uid) continue;
                const int msg_id = msg->GetMsgId();
                if (msg_id > 0 && !thread_data->GetMsgMapRef().contains(msg_id)) {
                    thread_data->AppendMsg(msg_id, msg);
                }
            }
        }

        if (is_group) {
            // the conversation item of a group chat is keyed by the thread id, not by a member uid
            auto item_iter = _chat_thread_items.find(thread_id);
            if (item_iter == _chat_thread_items.end()) {
                continue;
            }

            auto *group_wid = qobject_cast<ChatUserWid *>(
                ui->chat_user_list->itemWidget(item_iter.value()));
            if (group_wid) {
                group_wid->updateLastMsg(contents);
            }

            if (_state == ChatUIMode::ChatMode && _cur_chat_thread_id == thread_id) {
                UpdateChatMsg(sender_uid, contents);
            }
            continue;
        }

        auto friend_info = UserMgr::instance()->GetUserInfoById(sender_uid);
        if (!friend_info) {
            continue;
        }

        auto id_iter = _chat_id_items.find(sender_uid);
        if (id_iter == _chat_id_items.end()) {
            auto *chat_user_wid = new ChatUserWid();
            chat_user_wid->SetInfo(friend_info);

            auto *item = new QListWidgetItem();
            item->setSizeHint(chat_user_wid->sizeHint());

            ui->chat_user_list->insertItem(0, item);
            ui->chat_user_list->setItemWidget(item, chat_user_wid);

            id_iter = _chat_id_items.insert(sender_uid, item);
        }

        QListWidgetItem *item = id_iter.value();
        auto *chat_user_wid = qobject_cast<ChatUserWid *>(
            ui->chat_user_list->itemWidget(item));

        if (thread_id > 0) {
            if (!_chat_thread_items.contains(thread_id)) {
                _chat_thread_items.insert(thread_id, item);
            }

            if (chat_user_wid && !chat_user_wid->GetChatData()) {
                chat_user_wid->SetChatData(
                    std::make_shared<ChatThreadData>(sender_uid, thread_id, 0));
            }
        }

        if (chat_user_wid) {
            chat_user_wid->updateLastMsg(contents);
        }

        UserMgr::instance()->AppendFriendChatMsg(sender_uid, contents);

        if (_state == ChatUIMode::ChatMode &&
            thread_id > 0 &&
            _cur_chat_thread_id == thread_id) {
            UpdateChatMsg(sender_uid, contents);
        }
    }
}
void ChatDialog::slot_img_chat_msg(const QJsonObject &msg) {
    int thread_id = msg.value("thread_id").toInt();
    int from_uid = msg.value("fromuid").toInt();
    QString file_name = msg.value("name").toString();
    if (file_name.isEmpty()) {
        return;
    }

    const bool is_group = msg.value("is_group").toBool();

    ChatMsgType msg_type = ChatMsgType::PIC;
    int type_value = msg.value("type").toInt(-1);
    if (type_value == static_cast<int>(ChatMsgType::FILE)) {
        msg_type = ChatMsgType::FILE;
    } else if (type_value == static_cast<int>(ChatMsgType::PIC)) {
        msg_type = ChatMsgType::PIC;
    }

    auto friend_info = UserMgr::instance()->GetUserInfoById(from_uid);
    if (!friend_info) {
        return;
    }

    QJsonObject obj;
    obj["type"] = static_cast<int>(msg_type);
    obj["content"] = file_name;
    obj["owner_uid"] = from_uid;
    obj["message_id"] = msg.value("message_id").toInt();
    obj["status"] = msg.value("status").toInt();
    int new_msg_id = obj["message_id"].toInt();
    bool already_in_thread = false;
    if (thread_id > 0 && new_msg_id > 0) {
        auto thread_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
        if (thread_data && thread_data->GetMsgMapRef().contains(new_msg_id)) {
            already_in_thread = true;
        }
        else if (thread_data) {
            thread_data->AppendMsg(new_msg_id,
                std::make_shared<TextChatData>(new_msg_id, thread_id,
                    is_group ? ChatFormType::GROUP : ChatFormType::PRIVATE,
                    msg_type, file_name, from_uid,
                    msg.value("status").toInt(), msg.value("chat_time").toString()));
        }
    }

    QListWidgetItem *item = nullptr;
    if (is_group) {
        // group chat: reuse the group conversation item, never create a private one
        item = _chat_thread_items.value(thread_id, nullptr);
        if (!item) {
            return;
        }
    } else {
        auto id_iter = _chat_id_items.find(from_uid);
        if (id_iter == _chat_id_items.end()) {
            auto *new_wid = new ChatUserWid();
            new_wid->SetInfo(friend_info);

            auto *new_item = new QListWidgetItem();
            new_item->setSizeHint(new_wid->sizeHint());

            ui->chat_user_list->insertItem(0, new_item);
            ui->chat_user_list->setItemWidget(new_item, new_wid);

            id_iter = _chat_id_items.insert(from_uid, new_item);
        }
        item = id_iter.value();
    }

    auto *chat_user_wid = qobject_cast<ChatUserWid *>(
        ui->chat_user_list->itemWidget(item));

    if (thread_id > 0) {
        if (!_chat_thread_items.contains(thread_id)) {
            _chat_thread_items.insert(thread_id, item);
        }

        if (chat_user_wid && !chat_user_wid->GetChatData()) {
            chat_user_wid->SetChatData(
                std::make_shared<ChatThreadData>(is_group ? 0 : from_uid, thread_id, 0));
        }
    }

    if (chat_user_wid) {
        chat_user_wid->SetLastMsg(msg_type == ChatMsgType::FILE
            ? QStringLiteral("[文件]") : QStringLiteral("[图片]"));
    }

    if (!is_group) {
        QJsonArray contents;
        contents.append(obj);
        UserMgr::instance()->AppendFriendChatMsg(from_uid, contents);
    }

    if (_state == ChatUIMode::ChatMode &&
        thread_id > 0 &&
        _cur_chat_thread_id == thread_id) {
        if (already_in_thread) {
            return;
        }
        if (msg_type == ChatMsgType::FILE) {
            ui->chat_page->AppendRemoteFile(file_name, from_uid);
        } else {
            ui->chat_page->AppendRemoteImage(file_name, from_uid);
        }
    }
}
void ChatDialog::slot_chat_msg_rsp(std::vector<std::shared_ptr<TextChatData>> chat_msgs) {
    for (const auto& msg : chat_msgs) {
        if (!msg) continue;

        int thread_id = msg->GetThreadId();
        auto chat_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
        if (!chat_data) {
            chat_data = std::make_shared<ChatThreadData>(msg->GetSendUid(), thread_id, 0);
            UserMgr::instance()->AddChatThreadData(chat_data);
        }

        chat_data->MoveMsg(msg);

        auto item = _chat_thread_items.value(thread_id, nullptr);
        if (item) {
            auto *wid = qobject_cast<ChatUserWid*>(ui->chat_user_list->itemWidget(item));
            if (wid) {
                wid->SetLastMsg(chat_data->GetLastMsg());
            }
        }

        if (_state == ChatUIMode::ChatMode && _cur_chat_thread_id == thread_id) {
            RenderChatHistory(chat_data);
        }
    }
}

void ChatDialog::loadChatList() {

    QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(storage_dir);
    if (!dir.exists() && !dir.mkpath(storage_dir)) {
        qDebug() << "头像登录加载失败";
    }else {
        QString head_icon = UserMgr::instance()->GetIcon();
        if (!head_icon.isEmpty()) {
            QString avatar_path = dir.filePath(QString("avatars"));
            QString icon_path = QDir(avatar_path).filePath(head_icon);
            QPixmap pix(icon_path);
            if (!pix.isNull()) {
                ui->head_label->setPixmap(pix);
                ui->head_label->setScaledContents(true);
            } else {
                qWarning() << "头像加载失败:" << avatar_path;
            }
        } else {
            qDebug() << "icon    ''";
        }
    }
    QJsonObject jsonObj;
    auto uid = UserMgr::instance()->GetUid();
    jsonObj["uid"] = uid;
    // int last_chat_thread_id = UserMgr::instance()->GetLastChatThreadId();
    // jsonObj["thread_id"] = last_chat_thread_id;
    jsonObj["thread_id"] = 0;
    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    emit TCPMgr::instance()->sig_send_data(ReqId::ID_LOAD_CHAT_THREAD_REQ, jsonData);
    emit TCPFileMgr::instance()->sig_user_info_page_show_icon();
}

void ChatDialog::slot_load_chat_thread(bool load_more, int last_thread_id, std::vector<std::shared_ptr<ChatThreadInfo> > chat_threads) {
    for (auto& cti : chat_threads) {
        if (cti->_type == "group") {
            auto iter = _chat_thread_items.find(cti->_thread_id);
            if (iter != _chat_thread_items.end()) {
                qDebug() << cti->_thread_id << " group thread_id load fail";
                continue;
            }
            QListWidgetItem * item = new QListWidgetItem();
            auto* chat_item = new ChatUserWid(ChatFormType::GROUP);
            chat_item->SetThreadId(cti->_thread_id);
            chat_item->SetName(cti->_group_name.isEmpty() ? QString::fromUtf8("群聊") : cti->_group_name);
            item->setSizeHint(chat_item->sizeHint());
            ui->chat_user_list->insertItem(0, item);
            ui->chat_user_list->setItemWidget(item, chat_item);
            _chat_thread_items.insert(cti->_thread_id, item);

            auto chat_thread_data = std::make_shared<ChatThreadData>(0, cti->_thread_id, 0);
            chat_thread_data->SetGroupMembers(cti->_group_members);
            UserMgr::instance()->AddChatThreadData(chat_thread_data);
            chat_item->SetGroupUids(cti->_group_members);
            chat_item->SetChatData(chat_thread_data);
            fillGroupAvatars(chat_item, cti->_group_members);
        }else{
            int uid = UserMgr::instance()->GetUid();
            int other_uid = 0;
            if (uid == cti->_user1_id) {
                other_uid = cti->_user2_id;
            }else {
                other_uid = cti->_user1_id;
            }

            auto chat_thread_data = std::make_shared<ChatThreadData>(other_uid, cti->_thread_id, 0);
            UserMgr::instance()->AddChatThreadData(chat_thread_data);
            auto friend_info = UserMgr::instance()->GetUserInfoById(other_uid);
            if (!friend_info) {
                continue;
            }
            auto iter = _chat_id_items.find(other_uid);
            if (iter == _chat_id_items.end()) {
                auto *chat_user_wid = new ChatUserWid();
                chat_user_wid->SetChatData(chat_thread_data);
                QListWidgetItem* item = new QListWidgetItem;
                item->setSizeHint(chat_user_wid->sizeHint());
                ui->chat_user_list->insertItem(0, item);
                ui->chat_user_list->setItemWidget(item, chat_user_wid);
                _chat_thread_items.insert(cti->_thread_id, item);
                _chat_id_items.insert(other_uid, item);
            } else {
                _chat_thread_items.insert(cti->_thread_id, iter.value());

                auto *chat_user_wid = qobject_cast<ChatUserWid*>(
                    ui->chat_user_list->itemWidget(iter.value()));
                if (chat_user_wid) {
                    chat_user_wid->SetChatData(chat_thread_data);
                }
            }
        }
    }
    UserMgr::instance()->SetLastChatThreadId(last_thread_id);


    if (load_more) {
        QJsonObject jsonObj;
        auto uid = UserMgr::instance()->GetUid();
        jsonObj["uid"] = uid;
        jsonObj["thread_id"] = last_thread_id;


        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        //闁告瑦鍨块埀顑胯緶cp閻犲洭鏀遍惇鎵磼濡惧攧at server
        emit TCPMgr::instance()->sig_send_data(ReqId::ID_LOAD_CHAT_THREAD_REQ, jsonData);
        return;
    }

    load_chat_msg();
}

void ChatDialog::load_chat_msg() {
    _cur_load_chat = UserMgr::instance()->GetCurLoadData();
    if (!_cur_load_chat) return;

    QJsonObject jsonObj;
    jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
    jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    emit TCPMgr::instance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ, jsonData);
}

void ChatDialog::slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> user_info) {
    int thread_id = UserMgr::instance()->GetChatThreadByUid(user_info->_uid);
    auto iter = _chat_id_items.find(user_info->_uid);
    if (iter == _chat_id_items.end()) {
        auto chat_wid = new ChatUserWid();
        chat_wid->SetInfo(user_info);
        QListWidgetItem* item = new QListWidgetItem();
        item->setSizeHint(chat_wid->sizeHint());
        ui->chat_user_list->insertItem(0, item);
        ui->chat_user_list->setItemWidget(item, chat_wid);
        iter = _chat_id_items.insert(user_info->_uid, item);
    }
    if (thread_id != -1) {
        auto it = _chat_thread_items.find(thread_id);
        if (it == _chat_thread_items.end()) _chat_thread_items.insert(thread_id, iter.value());
        ui->chat_user_list->scrollToItem(iter.value());
        ui->chat_label->SetSelected(true);
        SetSelectChatItem(thread_id);

        auto item = iter.value();
        QWidget* widget = ui->chat_user_list->itemWidget(item);
        if (!widget) {
            qDebug() << "chat widget is null";
            return;
        }
        auto chat_user_wid = qobject_cast<ChatUserWid*>(widget);

        auto thread_data = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
        if (!thread_data) {
            thread_data = std::make_shared<ChatThreadData>(user_info->_uid, thread_id, 0);
            UserMgr::instance()->AddChatThreadData(thread_data);
        }

        chat_user_wid->SetChatData(thread_data);
        ui->chat_page->SetUserInfo(user_info);
        ui->chat_page->setChatData(thread_data);
        RenderChatHistory(thread_data);

        slot_side_chat();

        QTimer::singleShot(0, this, [this, item]() {
            ui->chat_user_list->setCurrentItem(item);
            ui->chat_user_list->scrollToItem(item, QAbstractItemView::PositionAtTop);
            ui->chat_user_list->setFocus();
        });
        return;
    }

    auto uid = UserMgr::instance()->GetUid();
    QJsonObject jsonObj;
    jsonObj["uid"] = uid;
    jsonObj["other_id"] = user_info->_uid;

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //闁告瑦鍨块埀顑胯緶cp閻犲洭鏀遍惇鎵磼濡惧攧at server
    emit TCPMgr::instance()->sig_send_data(ReqId::ID_CREATE_PRIVATE_CHAT_REQ, jsonData);
}

void ChatDialog::SetSelectChatItem(int thread_id)
{
    if (ui->chat_user_list->count() <= 0) {
        return;
    }

    auto find_iter = _chat_thread_items.find(thread_id);
    if (find_iter == _chat_thread_items.end()) {
        qDebug() << "thread_id [" << thread_id << "] not found, set curent row 0";
        ui->chat_user_list->setCurrentRow(0, QItemSelectionModel::NoUpdate);
        return;
    }

    ui->chat_user_list->setCurrentItem(find_iter.value(), QItemSelectionModel::NoUpdate);

    _cur_chat_thread_id = thread_id;
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
}
void ChatDialog::SetSelectChatPage(int thread_id) {
    if (ui->chat_user_list->count() <= 0) {
        return;
    }

    auto find_iter = _chat_thread_items.find(thread_id);
    if (find_iter == _chat_thread_items.end()) {
        return;
    }
    auto item = find_iter.value();
    QWidget* widget = ui->chat_user_list->itemWidget(find_iter.value());
    auto customItem = qobject_cast<ListItemBase*>(widget);
    auto chat_user_wid = qobject_cast<ChatUserWid*>(customItem);
    auto chat_data = chat_user_wid->GetChatData();
    ui->chat_page->setChatData(chat_data);
}

void ChatDialog::slot_con_item_clicked(QListWidgetItem *item) {
    auto con_wid = ui->con_user_list->itemWidget(item);
    if (!con_wid) {
        return;
    }
    auto con_item = qobject_cast<ConUserItem *>(con_wid);
    auto type = con_item->GetItemType();
    if (type == ListItemType::APPLY_FRIEND_ITEM) return;
    auto user_info = con_item->GetUserInfo();
    ui->friend_info_page->setInfo(user_info);
    ui->stackedWidget->setCurrentWidget(ui->friend_info_page);
    ui->friend_info_page->show();
}

void ChatDialog::slot_create_private_chat(int uid, int other_uid, int thread_id) {
    auto iter = _chat_id_items.find(other_uid);
    auto chat_thread_data = std::make_shared<ChatThreadData>(other_uid, thread_id, 0);
    if (iter == _chat_id_items.end()) {
        auto* chat_user_wid = new ChatUserWid();
        auto user_info = UserMgr::instance()->GetUserInfoById(other_uid);
        chat_user_wid->SetChatData(chat_thread_data);
        if (user_info) {
            chat_user_wid->SetInfo(user_info);
        }
        QListWidgetItem* item = new QListWidgetItem;
        item->setSizeHint(chat_user_wid->sizeHint());
        qDebug() << "chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
        ui->chat_user_list->insertItem(0, item);
        ui->chat_user_list->setItemWidget(item, chat_user_wid);
        _chat_thread_items.insert(thread_id, item);
        _chat_id_items.insert(other_uid, item);
    }else {
        _chat_thread_items.insert(thread_id, iter.value());
    }
    UserMgr::instance()->AddChatThreadData(chat_thread_data);

    auto friend_info = UserMgr::instance()->GetUserInfoById(other_uid);
    if (friend_info) {
        ui->chat_page->SetUserInfo(friend_info);
    }
    ui->chat_page->setChatData(chat_thread_data);

    ui->chat_label->SetSelected(true);
    SetSelectChatItem(thread_id);
    slot_side_chat();
}

void ChatDialog::slot_load_chat_msg(int thread_id, int msg_id, bool load_more, std::vector<std::shared_ptr<TextChatData>> msglists) {
    //鍔犺浇鑱婂ぉ淇℃伅
    if (!_cur_load_chat) {
        return;
    }

    for (auto& chat_msg : msglists) {
        _cur_load_chat->AppendMsg(chat_msg->GetMsgId(), chat_msg);
    }

    _cur_load_chat->SetLastMsgId(msg_id);

    // Update the conversation list preview with the latest loaded message.
    auto preview_item = _chat_thread_items.value(thread_id, nullptr);
    if (preview_item) {
        auto *preview_wid = qobject_cast<ChatUserWid*>(ui->chat_user_list->itemWidget(preview_item));
        if (preview_wid) {
            preview_wid->SetLastMsg(_cur_load_chat->GetLastMsg());
        }
    }

    // Open the first loaded conversation automatically after login.
    // if (_cur_chat_thread_id == -1) {
    //     auto other_uid = _cur_load_chat->GetOtherId();
    //     auto friend_info = UserMgr::instance()->GetUserInfoById(other_uid);
    //     if (friend_info) {
    //         _cur_chat_thread_id = thread_id;
    //         ui->chat_page->SetUserInfo(friend_info);
    //         ui->chat_page->setChatData(_cur_load_chat);
    //         ui->chat_page->show();
    //         ui->stackedWidget->setCurrentWidget(ui->chat_page);
    //         _state = ChatUIMode::ChatMode;
    //         ShowSearch(false);
    //         SetSelectChatItem(thread_id);
    //     }
    // }

    // If the currently open conversation is the one being loaded, refresh it.
    if (_state == ChatUIMode::ChatMode && _cur_chat_thread_id == thread_id) {
        RenderChatHistory(_cur_load_chat);
    }

    // load more messages if requested
    if (load_more) {
        //鍙戦€佽姹傜粰鏈嶅姟鍣?        //鍙戦€佽姹傞€昏緫
        QJsonObject jsonObj;
        jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
        jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

        //鍙戦€乼cp璇锋眰缁檆hat server
        emit TCPMgr::instance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ, jsonData);
        return;
    }

    //鑾峰彇涓嬩竴涓猚hat_thread
    _cur_load_chat = UserMgr::instance()->GetNextLoadData();
    // all chat threads loaded
    if (!_cur_load_chat) {
        //鏇存柊鑱婂ぉ鐣岄潰淇℃伅
        // SetSelectChatItem();
        // SetSelectChatPage();
        return;
    }

    //缁х画鍔犺浇涓嬩竴涓亰澶?    //鍙戦€佽姹傜粰鏈嶅姟鍣?    //鍙戦€佽姹傞€昏緫
    QJsonObject jsonObj;
    jsonObj["thread_id"] = _cur_load_chat->GetThreadId();
    jsonObj["message_id"] = _cur_load_chat->GetLastMsgId();

    QJsonDocument doc(jsonObj);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    //鍙戦€乼cp璇锋眰缁檆hat server
    emit TCPMgr::instance()->sig_send_data(ReqId::ID_LOAD_CHAT_MSG_REQ, jsonData);
}

void ChatDialog::slot_show_head_icon() {
    QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(storage_dir);
    if (!dir.exists() && !dir.mkpath(storage_dir)) {
        qDebug() << "头像登录加载失败";
    }else {
        QString head_icon = UserMgr::instance()->GetIcon();
        if (!head_icon.isEmpty()) {
            QString avatar_path = QDir(storage_dir).filePath(QString("avatars/") + head_icon);
            QPixmap pix(avatar_path);
            if (!pix.isNull()) {
                ui->head_label->setPixmap(pix);
                ui->head_label->setScaledContents(true);
            } else {
                qWarning() << "头像加载失败:" << avatar_path;
            }
        } else {
            qDebug() << "icon    ''";
        }
    }
}

void ChatDialog::slot_reset_icon(QString path) {
    UserMgr::instance()->ResetLabelIcon(path);
}

QPixmap LoadAvatarPix(const QString& iconFileName)
{
    if(iconFileName.isEmpty())
        return {};
    QString path = UserMgr::GetAvatarLocalPath(iconFileName);
    QPixmap pix(path);

    return pix;
}

void ChatDialog::fillGroupAvatars(ChatUserWid *chat_item, const QVector<int> &members)
{
    if (!chat_item) {
        return;
    }

    QVector<int> uids = members;
    const int my_uid = UserMgr::instance()->GetUid();
    if (my_uid > 0 && !uids.contains(my_uid)) {
        uids.prepend(my_uid);
    }

    QVector<QPixmap> avatarList;
    avatarList.reserve(uids.size());
    for (int uid : uids) {
        auto user_info = (uid == my_uid) ? UserMgr::instance()->GetUserInfo()
                                         : UserMgr::instance()->GetUserInfoById(uid);
        if (!user_info) {
            continue;
        }

        QPixmap pix = LoadAvatarPix(user_info->_icon);
        if (pix.isNull()) {
            UserMgr::instance()->EnsureAvatarDownloaded(user_info->_icon, uid);
        }
        avatarList.append(pix);
    }

    chat_item->SetGroupHead(2, 9, avatarList);
}

void ChatDialog::slot_refresh_group_avatars()
{
    for (auto it = _chat_thread_items.constBegin(); it != _chat_thread_items.constEnd(); ++it) {
        auto *chat_item = qobject_cast<ChatUserWid *>(
            ui->chat_user_list->itemWidget(it.value()));
        if (!chat_item || chat_item->GetChatType() != ChatFormType::GROUP) {
            continue;
        }

        QVector<int> uids = chat_item->GetGroupUids();
        if (uids.isEmpty()) {
            auto thread_data = UserMgr::instance()->GetChatThreadDataByThreadId(it.key());
            if (thread_data) {
                uids = thread_data->GetGroupMembers();
            }
        }
        fillGroupAvatars(chat_item, uids);
    }
}

void ChatDialog::slot_show_add_group() {
    _applygroup_chat->clearItems();
    _applygroup_chat->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    _applygroup_chat->setModal(true);
    _applygroup_chat->show();

    auto friend_list = UserMgr::instance()->GetFriendList();
    for (const auto& f : friend_list) {
        addgroupitem* groupItem = new addgroupitem;
        groupItem->SetUserInfo(f);
        _applygroup_chat->addItem(groupItem);
    }
}

void ChatDialog::slot_group_chat_item(QVector<int> members, int thread_id) {
    if (members.isEmpty() || thread_id == -1) return;

    auto iter = _chat_thread_items.find(thread_id);
    if (iter != _chat_thread_items.end()) return;
    QListWidgetItem* item = new QListWidgetItem;
    auto chat_item = new ChatUserWid(ChatFormType::GROUP);
    chat_item->SetThreadId(thread_id);
    chat_item->SetName("群聊");
    item->setSizeHint(chat_item->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_item);

    _chat_thread_items.insert(thread_id, item);
    chat_item->SetGroupUids(members);
    if (!UserMgr::instance()->GetChatThreadDataByThreadId(thread_id)) {
        auto chat_thread_data = std::make_shared<ChatThreadData>(0, thread_id, 0);
        chat_thread_data->SetGroupMembers(members);
        UserMgr::instance()->AddChatThreadData(chat_thread_data);
        chat_item->SetChatData(chat_thread_data);
    }

    fillGroupAvatars(chat_item, members);

}

void ChatDialog::slot_notify_group_chat_item(QVector<int> members, int host_uid, int thread_id) {
    if (members.isEmpty() || thread_id == -1) return;

    auto iter = _chat_thread_items.find(thread_id);
    if (iter != _chat_thread_items.end()) return;
    QListWidgetItem* item = new QListWidgetItem;
    auto chat_item = new ChatUserWid(ChatFormType::GROUP);
    chat_item->SetThreadId(thread_id);
    chat_item->SetName("群聊");
    item->setSizeHint(chat_item->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_item);

    _chat_thread_items.insert(thread_id, item);
    chat_item->SetGroupUids(members);
    if (!UserMgr::instance()->GetChatThreadDataByThreadId(thread_id)) {
        auto chat_thread_data = std::make_shared<ChatThreadData>(0, thread_id, 0);
        chat_thread_data->SetGroupMembers(members);
        UserMgr::instance()->AddChatThreadData(chat_thread_data);
        chat_item->SetChatData(chat_thread_data);
    }

    fillGroupAvatars(chat_item, members);

}

void ChatDialog::slot_more_select() {
    ui->more_label->ClearState();
    if (_more_label_meum)
    {
        if (_more_label_meum->isVisible())
        {
            _more_label_meum->close();
            return;
        }
    }


    QPoint globalPos = ui->more_label->mapToGlobal(QPoint(0, ui->more_label->height()));
    _more_label_meum->popup(globalPos);
}

void ChatDialog::slot_logout() {
    // QJsonObject jsonObj;
    // jsonObj["uid"] = UserMgr::instance()->GetUid();
    // QJsonDocument doc = QJsonDocument(jsonObj);
    // QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    // TCPMgr::instance()->sig_send_data(ID_USER_LOGOUT_REQ, jsonData);

    emit sig_user_logout();
}

void ChatDialog::slot_show_red_point_chat(int thread_id) {
    auto iter = _chat_thread_items.find(thread_id);
    if (iter == _chat_thread_items.end()) return;
    auto *item = iter.value();
    auto *widget = ui->chat_user_list->itemWidget(item);
    if (!widget) return;

    if (_state != ChatUIMode::ChatMode) {
        ui->chat_label->ShowRedPoint(true);
    }
    auto item_chat = qobject_cast<ChatUserWid*>(widget);
    if (_cur_chat_thread_id != thread_id) item_chat->ShowRedPoint(true);

}