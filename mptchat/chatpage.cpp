#include "chatpage.h"
#include "ui_ChatPage.h"
#include "chatview.h"
#include <QStyleOption>
#include <QPainter>
#include <QScrollBar>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include "FileBubble.h"
#include "UserMgr.h"
#include "global.h"
#include "messagebubble.h"
#include "PictureBubble.h"
#include "TCPMgr.h"
#include "TCPFileMgr.h"
#include "ChatFriendInfoShow.h"
#include "ChatGroupInfoShow.h"
#include "AudioRecorder.h"
#include "SherpaOnnxRecognizer.h"
#include <QTextCursor>
ChatPage::ChatPage(QWidget *parent) : QWidget(parent), ui(new Ui::ChatPage) {
    ui->setupUi(this);

    ui->show_friend_or_member_label->setProperty("state", "normal");
    ui->show_friend_or_member_label->SetState("normal", "", "", "selected", "", "");
    _private_info = new ChatFriendInfoShow(this);
    _group_info = new ChatGroupInfoShow(this);
    connect(ui->show_friend_or_member_label, &ClickedLabel::clicked, this, &ChatPage::slot_display_friend_or_group_info);



    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_chatDialog_head_icon,
            this, &ChatPage::refreshAvatars);
    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_img_chat_downloaded,
            this, &ChatPage::onImgDownloaded);
    // an avatar finished downloading: repaint the bubbles of group members
    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_avatar_downloaded,
            this, &ChatPage::refreshAvatars);

    connect(ui->send_btn,   &QPushButton::clicked, this, &ChatPage::onSendClicked);

    // ==================== 测试代码 START ====================
    connect(ui->receive_btn, &QPushButton::clicked, this, &ChatPage::onReceiveClicked);
    // ==================== 测试代码 END   ====================

    auto *asr = SherpaOnnxRecognizer::instance().get();
    _recorder = new AudioRecorder(this);

    // 模型要加载约 1 秒，好了之前先禁用，免得用户点了没反应
    ui->recognizer_label->setEnabled(false);
    ui->recognizer_label->SetState("normal", "", "", "selected", "", "");

    connect(ui->recognizer_label, &ClickedLabel::clicked, this,
            [this, asr](QString, ClickLbState state) {
        if (state == ClickLbState::Selected) {
            // ---- 开始说话 ----
            if (!_recorder->isUsable()) {
                ui->chat_edit->setPlaceholderText(QStringLiteral("没有可用的麦克风"));
                ui->recognizer_label->SetCurState(ClickLbState::Normal);
                ui->chat_edit->setPlaceholderText(QStringLiteral(""));
                return;
            }
            ui->recognizer_label->setText(QStringLiteral("停止"));
            ui->chat_edit->setPlaceholderText(QStringLiteral("正在聆听…"));
            QMetaObject::invokeMethod(asr, "slotCreateStream", Qt::QueuedConnection);
            _recorder->slotStart();
        } else {
            // ---- 结束说话 ----
            ui->recognizer_label->setText(QStringLiteral("转文字"));
            ui->chat_edit->setPlaceholderText(QStringLiteral(""));
            _recorder->slotStop();
            QMetaObject::invokeMethod(asr, "slotFinishInput", Qt::QueuedConnection);
        }
    });

    // 麦克风数据 -> 识别器（跨线程，自动走队列连接）
    connect(_recorder, &AudioRecorder::sigAudioReady, asr,
            &SherpaOnnxRecognizer::slotAcceptWaveform);

    connect(_recorder, &AudioRecorder::sigError, this, [this](const QString &msg) {
        ui->chat_edit->setPlaceholderText(msg);
    });

    // 识别结果 -> 界面
    connect(asr, &SherpaOnnxRecognizer::sigPartialText, this, &ChatPage::onPartialText);
    connect(asr, &SherpaOnnxRecognizer::sigSentence,    this, &ChatPage::onSentence);
    connect(asr, &SherpaOnnxRecognizer::sigInitFinished, this, &ChatPage::onAsrInitFinished);
}

ChatPage::~ChatPage() {
    delete ui;
}

void ChatPage::SetUserInfo(std::shared_ptr<UserInfo> info) {
    _user_info = info;
    _group_members_uid.clear();
    if (info) {
        ui->title_label->setText(info->_name);
    }
}

void ChatPage::setChatData(std::shared_ptr<ChatThreadData> chat_data) {
    _chat_data = chat_data;
}

void ChatPage::ScrollToBottom() {
    ui->chat_data_list->scrollToBottom();
}

QPixmap ChatPage::loadAvatarPixmap(const QString &iconFileName) const {
    if (iconFileName.isEmpty()) {
        return QPixmap();
    }

    const QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(storage_dir);
    if (!dir.exists("avatars")) {
        return QPixmap();
    }

    QPixmap pix(dir.filePath("avatars/" + iconFileName));
    if (pix.isNull()) {
        return QPixmap();
    }

    return pix.scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ChatPage::resolveSender(int send_uid, ChatRole role,
                             QString &name, QPixmap &icon) const {
    if (role == ChatRole::Self) {
        auto self_info = UserMgr::instance()->GetUserInfo();
        name = self_info ? self_info->_name : QString();
        icon = self_info ? loadAvatarPixmap(self_info->_icon) : QPixmap();
        return;
    }

    std::shared_ptr<UserInfo> other_info;
    if (isGroupChat()) {
        other_info = UserMgr::instance()->GetUserInfoById(send_uid);
        if (other_info) {
            // group members are not necessarily friends, so their avatar may not be cached yet
            UserMgr::instance()->EnsureAvatarDownloaded(other_info->_icon, send_uid);
        }
    } else {
        other_info = _user_info;
    }

    name = other_info ? other_info->_name : QString();
    icon = other_info ? loadAvatarPixmap(other_info->_icon) : QPixmap();
}

void ChatPage::refreshAvatars() {
    const auto bubbles = ui->chat_data_list->messageBubbles();
    for (auto *bubble : bubbles) {
        if (!bubble) {
            continue;
        }

        const int sender_uid = bubble->property("sender_uid").toInt();
        QString sender_name;
        QPixmap avatar;
        resolveSender(sender_uid, bubble->role(), sender_name, avatar);
        if (!avatar.isNull()) {
            bubble->setUserIcon(avatar);
        }
    }
}

void ChatPage::onSendClicked() {
    if (!_chat_data) return;
    if (!hasActiveChat()) return;

    auto user_info = UserMgr::instance()->GetUserInfo();
    if (!user_info) return;

    auto name = user_info->_name;
    auto icon = user_info->_icon;

    const QVector<std::shared_ptr<MsgInfo>> &msgList = ui->chat_edit->getMsgList();
    if (msgList.isEmpty()) return;

    int thread_id = _chat_data->GetThreadId();

    const int touid = _user_info ? _user_info->_uid : 0;
    const ChatFormType form_type = isGroupChat() ? ChatFormType::GROUP : ChatFormType::PRIVATE;
    if (thread_id <= 0) return;

    QJsonObject jsonObj;
    QJsonArray textArray;
    int txt_size = 0;

    for (int i = 0; i < msgList.size(); ++i) {
        if (msgList[i]->_content_or_url.length() > 1024) continue;
        auto Type = msgList[i]->_type;

        if (Type == ChatMsgType::TEXT) {
            auto *bubble = new MessageBubble(ChatRole::Self, msgList[i]->_content_or_url);
            bubble->setUserName(name);

            QPixmap avatar = loadAvatarPixmap(icon);
            if (!avatar.isNull()) {
                bubble->setUserIcon(avatar);
            }

            ui->chat_data_list->appendChatItem(bubble);

            if (txt_size + msgList[i]->_content_or_url.length() > 1024) {
                jsonObj["fromuid"] = user_info->_uid;
                jsonObj["touid"] = touid;
                jsonObj["thread_id"] = thread_id;
                jsonObj["text_array"] = textArray;
                QJsonDocument jsonDoc(jsonObj);
                QByteArray jsonData = jsonDoc.toJson(QJsonDocument::Compact);
                jsonObj = QJsonObject();
                textArray = QJsonArray();
                txt_size = 0;
                emit TCPMgr::instance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
            }
            txt_size += msgList[i]->_content_or_url.length();
            QJsonObject obj;
            QByteArray utf8Message = msgList[i]->_content_or_url.toUtf8();
            auto content = QString::fromUtf8(utf8Message);

            obj["content"] = content;
            QString msg_unique_id = QUuid::createUuid().toString();
            obj["unique_id"] = msg_unique_id;
            textArray.append(obj);

            auto txt_msg = std::make_shared<TextChatData>(msg_unique_id, thread_id, form_type,
                ChatMsgType::TEXT, content, user_info->_uid, 0);

            _chat_data->AppendUnRspMsg(msg_unique_id, txt_msg);
        }else if (Type == ChatMsgType::PIC) {
            if (txt_size) {
                jsonObj["fromuid"] = user_info->_uid;
                jsonObj["touid"] = touid;
                jsonObj["thread_id"] = thread_id;
                jsonObj["text_array"] = textArray;
                QJsonDocument doc(jsonObj);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                txt_size = 0;
                textArray = QJsonArray();
                jsonObj = QJsonObject();
                emit TCPMgr::instance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
            }

            QPixmap preview = msgList[i]->_preview_pix;
            if (preview.isNull()) {
                preview = QPixmap(msgList[i]->_content_or_url);
            }
            auto *pBubble = new PictureBubble(preview, ChatRole::Self);
            pBubble->setUserName(name);
            QPixmap avatar = loadAvatarPixmap(icon);
            if (!avatar.isNull()) {
                pBubble->setUserIcon(avatar);
            }
            ui->chat_data_list->appendChatItem(pBubble);

            QString img_unique_id = QUuid::createUuid().toString();
            auto img_msg = std::make_shared<ImgChatData>(msgList[i], img_unique_id, thread_id, form_type,
                ChatMsgType::PIC, user_info->_uid, 0);
            _chat_data->AppendUnRspMsg(img_unique_id, img_msg);
            jsonObj["fromuid"] = user_info->_uid;
            jsonObj["touid"] = touid;
            jsonObj["thread_id"] = thread_id;
            jsonObj["md5"] = msgList[i]->_md5;
            jsonObj["name"] = msgList[i]->_unique_name;
            jsonObj["token"] = UserMgr::instance()->GetToken();
            jsonObj["unique_id"] = img_unique_id;
            jsonObj["total_size"] = msgList[i]->_total_size;
            UserMgr::instance()->AddTransFile(msgList[i]->_unique_name, msgList[i]);
            QJsonDocument doc(jsonObj);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

            emit TCPMgr::instance()->sig_send_data(ID_IMG_CHAT_MSG_REQ, jsonData);
            jsonObj = QJsonObject();
        }else if (Type == ChatMsgType::FILE) {
            if (txt_size) {
                jsonObj["fromuid"] = user_info->_uid;
                jsonObj["touid"] = touid;
                jsonObj["thread_id"] = thread_id;
                jsonObj["text_array"] = textArray;
                QJsonDocument doc(jsonObj);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                txt_size = 0;
                textArray = QJsonArray();
                jsonObj = QJsonObject();
                emit TCPMgr::instance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
            }
            QFile file(msgList[i]->_content_or_url);
            if (file.open(QIODevice::ReadOnly)) {
                QString file_unique_id = QUuid::createUuid().toString();
                FileBubble * pBubble = new FileBubble(ChatRole::Self, msgList[i]->_unique_name,
                                                      msgList[i]->_content_or_url, user_info->_uid, true);
                pBubble->setUserName(name);
                QPixmap avatar = loadAvatarPixmap(icon);
                if (!avatar.isNull()) {
                    pBubble->setUserIcon(avatar);
                }
                ui->chat_data_list->appendChatItem(pBubble);
                auto file_chat_data = std::make_shared<FileChatData>(msgList[i], file_unique_id, thread_id, form_type,
                    ChatMsgType::FILE, user_info->_uid, 0);
                _chat_data->AppendUnRspMsg(file_unique_id, file_chat_data);
                jsonObj["fromuid"] = user_info->_uid;
                jsonObj["touid"] = touid;
                jsonObj["thread_id"] = thread_id;
                jsonObj["md5"] = msgList[i]->_md5;
                jsonObj["name"] = msgList[i]->_unique_name;
                jsonObj["token"] = UserMgr::instance()->GetToken();
                jsonObj["unique_id"] = file_unique_id;
                jsonObj["total_size"] = msgList[i]->_total_size;
                UserMgr::instance()->AddTransFile(msgList[i]->_unique_name, msgList[i]);
                QJsonDocument doc(jsonObj);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

                emit TCPMgr::instance()->sig_send_data(ID_FILE_CHAT_MSG_REQ, jsonData);
                jsonObj = QJsonObject();
            }
        }
    }

    if (!textArray.isEmpty()) {
        jsonObj["text_array"] = textArray;
        jsonObj["fromuid"] = user_info->_uid;
        jsonObj["touid"] = touid;
        jsonObj["thread_id"] = thread_id;
        QJsonDocument doc(jsonObj);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        txt_size = 0;
        textArray = QJsonArray();
        jsonObj = QJsonObject();
        emit TCPMgr::instance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ, jsonData);
    }
    ui->chat_edit->clearGetMsgList();
    ui->chat_edit->clear();
}
void ChatPage::AppendOtherMessage(int sender_uid, const QJsonArray &contents) {
    if (!hasActiveChat()) return;

    QString name;
    QPixmap icon;
    resolveSender(sender_uid, ChatRole::Other, name, icon);

    for (const QJsonValue &val : contents) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();

        if (obj.value("type").toInt() == static_cast<int>(ChatMsgType::FILE)) {
            QString file_name = obj.value("content").toString();
            int owner_uid = obj.value("owner_uid").toInt();
            appendFileMessage(file_name, ChatRole::Other, name, icon, owner_uid);
            continue;
        }

        if (obj.value("type").toInt() == static_cast<int>(ChatMsgType::PIC)) {
            QString image_name = obj.value("content").toString();
            int owner_uid = obj.value("owner_uid").toInt();
            appendImageMessage(image_name, ChatRole::Other, name, icon, owner_uid);
            continue;
        }

        QString text = obj.value("content").toString();
        if (text.isEmpty()) continue;

        auto *bubble = new MessageBubble(ChatRole::Other, text);
        bubble->setUserName(name);
        bubble->setProperty("sender_uid", sender_uid);
        if (!icon.isNull()) {
            bubble->setUserIcon(icon);
        }
        ui->chat_data_list->appendChatItem(bubble);
    }
}

void ChatPage::ClearChatMessages() {
    _pending_pictures.clear();
    ui->chat_data_list->clearChatItems();
}

void ChatPage::AppendHistoryMessages(const std::vector<std::shared_ptr<TextChatData>>& msgs) {
    if (!hasActiveChat()) return;

    auto self_info = UserMgr::instance()->GetUserInfo();
    int my_uid = self_info ? self_info->_uid : -1;



    for (const auto& msg : msgs) {
        if (!msg) continue;

        QString text = msg->GetMsgContent();
        if (text.isEmpty()) continue;

        bool is_self = (msg->GetSendUid() == my_uid);
        ChatRole role = is_self ? ChatRole::Self : ChatRole::Other;
        QString sender_name;
        QPixmap sender_icon;
        resolveSender(msg->GetSendUid(), role, sender_name, sender_icon);

        if (msg->GetMsgType() == ChatMsgType::FILE) {
            appendFileMessage(QFileInfo(text).fileName(), role, sender_name, sender_icon, msg->GetSendUid());
            continue;
        }

        if (msg->GetMsgType() == ChatMsgType::PIC) {
            appendImageMessage(QFileInfo(text).fileName(), role, sender_name, sender_icon, msg->GetSendUid());
            continue;
        }

        auto *bubble = new MessageBubble(role, text);
        bubble->setUserName(sender_name);
        bubble->setProperty("sender_uid", msg->GetSendUid());
        if (!sender_icon.isNull()) {
            bubble->setUserIcon(sender_icon);
        }
        ui->chat_data_list->appendChatItem(bubble);
    }
}

void ChatPage::AppendPendingMessages(const QMap<QString, std::shared_ptr<ChatDataBase>>& pending) {
    if (!hasActiveChat()) return;

    auto self_info = UserMgr::instance()->GetUserInfo();
    QString self_name = self_info ? self_info->_name : QString();
    QPixmap self_icon = self_info ? loadAvatarPixmap(self_info->_icon) : QPixmap();

    for (auto it = pending.cbegin(); it != pending.cend(); ++it) {
        auto msg = it.value();
        if (!msg) continue;
        if (msg->GetMsgType() == ChatMsgType::TEXT) {
            continue;
        }

        if (msg->GetMsgType() == ChatMsgType::PIC) {
            auto img = std::dynamic_pointer_cast<ImgChatData>(msg);
            QPixmap pix;
            QString image_name;
            if (img && img->_msg_info) {
                pix = img->_msg_info->_preview_pix;
                image_name = img->_msg_info->_unique_name;
            }
            auto *bubble = new PictureBubble(pix, ChatRole::Self);
            bubble->setUserName(self_name);
            bubble->setImageName(QFileInfo(image_name).fileName());
            if (!self_icon.isNull()) {
                bubble->setUserIcon(self_icon);
            }
            ui->chat_data_list->appendChatItem(bubble);
            continue;
        }

        if (msg->GetMsgType() == ChatMsgType::FILE) {
            auto file_msg = std::dynamic_pointer_cast<FileChatData>(msg);
            QString file_name = msg->GetMsgContent();
            QString file_path = file_name;
            if (file_msg && file_msg->_msg_info) {
                file_name = file_msg->_msg_info->_unique_name;
                file_path = file_msg->_msg_info->_content_or_url;
            }
            auto *bubble = new FileBubble(ChatRole::Self, QFileInfo(file_name).fileName(), file_path,
                                          msg->GetSendUid(), true);
            bubble->setUserName(self_name);
            if (!self_icon.isNull()) {
                bubble->setUserIcon(self_icon);
            }
            ui->chat_data_list->appendChatItem(bubble);
            continue;
        }

        auto *bubble = new MessageBubble(ChatRole::Self, msg->GetMsgContent());
        bubble->setUserName(self_name);
        if (!self_icon.isNull()) {
            bubble->setUserIcon(self_icon);
        }
        ui->chat_data_list->appendChatItem(bubble);
    }
 }

void ChatPage::AppendChatItems(const std::vector<std::shared_ptr<ChatDataBase>>& msgs) {
    if (!hasActiveChat()) return;

    auto self_info = UserMgr::instance()->GetUserInfo();
    int my_uid = self_info ? self_info->_uid : -1;



    for (const auto& msg : msgs) {
        if (!msg) continue;

        bool is_self = (msg->GetSendUid() == my_uid);
        ChatRole role = is_self ? ChatRole::Self : ChatRole::Other;
        QString sender_name;
        QPixmap sender_icon;
        resolveSender(msg->GetSendUid(), role, sender_name, sender_icon);

        ChatMsgType type = msg->GetMsgType();

        if (type == ChatMsgType::FILE) {
            auto file_msg = std::dynamic_pointer_cast<FileChatData>(msg);
            if (file_msg && file_msg->_msg_info) {
                QString file_name = file_msg->_msg_info->_unique_name;
                QString file_path = file_msg->_msg_info->_content_or_url;
                auto *bubble = new FileBubble(role, QFileInfo(file_name).fileName(), file_path,
                                              msg->GetSendUid(), true);
                bubble->setUserName(sender_name);
                bubble->setProperty("sender_uid", msg->GetSendUid());
                if (!sender_icon.isNull()) {
                    bubble->setUserIcon(sender_icon);
                }
                ui->chat_data_list->appendChatItem(bubble);
            } else {
                QString file_name = msg->GetMsgContent();
                appendFileMessage(QFileInfo(file_name).fileName(), role, sender_name, sender_icon,
                                  msg->GetSendUid());
            }
            continue;
        }

        if (type == ChatMsgType::PIC) {
            auto img_msg = std::dynamic_pointer_cast<ImgChatData>(msg);
            if (img_msg && img_msg->_msg_info) {
                QPixmap pix = img_msg->_msg_info->_preview_pix;
                QString image_name = img_msg->_msg_info->_unique_name;
                auto *bubble = new PictureBubble(pix, role);
                bubble->setUserName(sender_name);
                bubble->setImageName(QFileInfo(image_name).fileName());
                bubble->setProperty("sender_uid", msg->GetSendUid());
                if (!sender_icon.isNull()) {
                    bubble->setUserIcon(sender_icon);
                }
                ui->chat_data_list->appendChatItem(bubble);
            } else {
                QString image_name = msg->GetMsgContent();
                appendImageMessage(QFileInfo(image_name).fileName(), role, sender_name, sender_icon,
                                   msg->GetSendUid());
            }
            continue;
        }

        QString text = msg->GetMsgContent();
        if (text.isEmpty()) continue;

        auto *bubble = new MessageBubble(role, text);
        bubble->setUserName(sender_name);
        bubble->setProperty("sender_uid", msg->GetSendUid());
        if (!sender_icon.isNull()) {
            bubble->setUserIcon(sender_icon);
        }
        ui->chat_data_list->appendChatItem(bubble);
    }
}

 void ChatPage::AppendRemoteFile(const QString &file_name, int owner_uid) {
    if (!hasActiveChat() || file_name.isEmpty()) {
        return;
    }
    const ChatRole role = (owner_uid == UserMgr::instance()->GetUid()) ? ChatRole::Self : ChatRole::Other;
    QString sender_name;
    QPixmap sender_icon;
    resolveSender(owner_uid, role, sender_name, sender_icon);
    appendFileMessage(file_name, role, sender_name, sender_icon, owner_uid);
}

void ChatPage::appendFileMessage(const QString &file_name, ChatRole role,
                                  const QString &user_name, const QPixmap &user_icon,
                                  int owner_uid) {
    const QString clean_name = QFileInfo(file_name).fileName();
    if (clean_name.isEmpty()) {
        return;
    }

    QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (storage_dir.isEmpty()) {
        storage_dir = QDir::home().filePath(".mptchat");
    }
    QString cache_dir = QDir(storage_dir).filePath("chat_files");
    QDir dir(cache_dir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString cache_path = QDir(cache_dir).filePath(clean_name);

    auto *bubble = new FileBubble(role, clean_name, cache_path, owner_uid, false);
    bubble->setUserName(user_name);
    bubble->setProperty("sender_uid", owner_uid);
    if (!user_icon.isNull()) {
        bubble->setUserIcon(user_icon);
    }
    ui->chat_data_list->appendChatItem(bubble);
}

void ChatPage::AppendRemoteImage(const QString &image_name, int owner_uid) {
    if (!hasActiveChat() || image_name.isEmpty()) {
        return;
    }

    const ChatRole role = (owner_uid == UserMgr::instance()->GetUid()) ? ChatRole::Self : ChatRole::Other;
    QString sender_name;
    QPixmap sender_icon;
    resolveSender(owner_uid, role, sender_name, sender_icon);
    appendImageMessage(image_name, role, sender_name, sender_icon, owner_uid);
}

void ChatPage::appendImageMessage(const QString &image_name, ChatRole role,
                                  const QString &user_name, const QPixmap &user_icon,
                                  int owner_uid) {
    const QString clean_name = QFileInfo(image_name).fileName();
    if (clean_name.isEmpty()) {
        return;
    }

    QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString cache_dir = QDir(storage_dir).filePath("chat_images");
    //QDir().mkpath(cache_dir);
    QString cache_path = QDir(cache_dir).filePath(clean_name);
    QString temp_path = QDir::temp().filePath(clean_name);

    QPixmap pix(cache_path);
    if (pix.isNull()) {
        pix = QPixmap(temp_path);
    }

    auto *bubble = new PictureBubble(pix, role);
    bubble->setImageName(clean_name);
    bubble->setUserName(user_name);
    bubble->setProperty("sender_uid", owner_uid);
    if (!user_icon.isNull()) {
        bubble->setUserIcon(user_icon);
    }
    ui->chat_data_list->appendChatItem(bubble);

    if (pix.isNull()) {
        _pending_pictures[clean_name].append(bubble);
        auto download = std::make_shared<DownloadInfo>(clean_name, cache_path, owner_uid);
        UserMgr::instance()->AddDownloadFile(clean_name, download);
        TCPFileMgr::instance()->SendDownloadInfo(download);
    }
}

void ChatPage::onImgDownloaded(const QString &name, const QString &localPath) {
    QPixmap pix(localPath);
    if (pix.isNull()) {
        return;
    }

    auto iter = _pending_pictures.find(name);
    if (iter == _pending_pictures.end()) {
        return;
    }

    for (auto *bubble : iter.value()) {
        if (bubble) {
            bubble->setPicture(pix);
        }
    }

    _pending_pictures.remove(name);
}

// ==================== 测试代码 START ====================
void ChatPage::onReceiveClicked() {

}
// ==================== 测试代码 END   ====================

void ChatPage::paintEvent(QPaintEvent *event) {
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void ChatPage::SetTitleName(QString name) {
    ui->title_label->setText(name);
}

void ChatPage::setGroupUids(const QVector<int> &uids) {
    _group_members_uid = uids;
    if (!_group_members_uid.isEmpty()) {
        _user_info = nullptr;
    }
}

void ChatPage::slot_display_friend_or_group_info() {
    if (!hasActiveChat()) return;
    if (_user_info) {
        qDebug() << "this is private chatPage";
        _private_info->setWindowTitle(ui->title_label->text());
        _private_info->SetPrivateChatInfo(_user_info->_icon);
        _private_info->show();

    }else {
        _group_info->setWindowTitle(ui->title_label->text());
        //_group_info->SetPrivateChatInfo(_user_info->_icon);
        _group_info->show();
        qDebug() << "this is group chatPage";
    }
}

void ChatPage::onAsrInitFinished(bool ok)
{
    ui->recognizer_label->setEnabled(ok);
    if (!ok) {
        ui->chat_edit->setPlaceholderText(QStringLiteral("语音识别初始化失败"));
    }
}

void ChatPage::onPartialText(const QString &text)
{
    // 用占位符做实时预览：只显示、不进文档
    ui->chat_edit->setPlaceholderText(
        text.isEmpty() ? QStringLiteral("正在聆听…") : text);
}

void ChatPage::onSentence(const QString &text)
{
    ui->chat_edit->setPlaceholderText(QString());   // 清掉预览
    if (text.isEmpty()) return;

    QTextCursor c = ui->chat_edit->textCursor();
    c.movePosition(QTextCursor::End);
    ui->chat_edit->setTextCursor(c);
    ui->chat_edit->insertPlainText(text + ",");           // 正式写进输入框
}