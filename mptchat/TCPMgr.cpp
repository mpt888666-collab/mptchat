//
// Created by mpt on 2026/7/6.
//

#include "TCPMgr.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include "UserMgr.h"
#include <QtEndian>

#include "TCPFileMgr.h"

TCPThread::TCPThread() {
    _tcp_thread = new QThread();
    _tcp_thread->setObjectName("tcp_thread");

    auto mgr = TCPMgr::instance().get();

    mgr->moveToThread(_tcp_thread);
    mgr->socket()->moveToThread(_tcp_thread);

    qDebug() << "TCPMgr thread object:" << mgr->thread();
    qDebug() << "socket thread object:" << mgr->socket()->thread();
    qDebug() << "tcp_thread object:   " << _tcp_thread;

    _tcp_thread->start();

    QMetaObject::invokeMethod(mgr, []() {
        qDebug() << "worker thread name:" << QThread::currentThread()->objectName()
                 << "worker thread id:" << QThread::currentThreadId();
    }, Qt::QueuedConnection);
}

TCPThread::~TCPThread() {
    if (!_tcp_thread)
        return;

    auto mgr = TCPMgr::instance().get();

    if (mgr && _tcp_thread->isRunning()) {
        QMetaObject::invokeMethod(
            mgr, "CloseConnection",
            Qt::BlockingQueuedConnection);
    }

    _tcp_thread->quit();
    _tcp_thread->wait();
    delete _tcp_thread;
}

TCPMgr::TCPMgr() : _host(""), _port(0), _b_recv_pending(false), _message_id(0), _message_len(0){
    initHandlers();
    connect(&_socket, &QTcpSocket::connected, this, [&]() {
            qDebug() << "Connected to server!";
            emit sig_connect_success(true);
    });

    connect(&_socket, &QTcpSocket::readyRead, [&]() {
        _buffer.append(_socket.readAll());

        while (true) {
            if (!_b_recv_pending) {
                if (_buffer.size() < 4) return;

                const char* ptr = _buffer.constData();
                _message_id = qFromBigEndian<qint16>(ptr);
                _message_len = qFromBigEndian<qint16>(ptr + 2);
                _buffer.remove(0, 4);
                qDebug() << "ID:" << _message_id << "Len:" << _message_len;
            }

            if (_buffer.size() < _message_len) {
                _b_recv_pending = true;
                return;
            }

            _b_recv_pending = false;

            QByteArray messageBody = _buffer.left(_message_len);
            _buffer.remove(0, _message_len);

            handleMsg(ReqId(_message_id), _message_len, messageBody);
        }
    });


    connect(&_socket, &QTcpSocket::bytesWritten, this, [&](qint64 bytes) {
        _send_bytes += bytes;
        if (_send_bytes < _current_block.size()) {
            return;
        }
        if (!_send_queue.isEmpty()) {
            _current_block = _send_queue.dequeue();
            _send_bytes = 0;
            _socket.write(_current_block);
            return;
        }
        _current_block.clear();
        _pending = false;
        _send_bytes = 0;
    });
    connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
           Q_UNUSED(socketError)
           qDebug() << "Chat Error:" << _socket.errorString();
    });

    connect(&_socket, &QTcpSocket::disconnected, [&]() {
            qDebug() << "Chat Disconnected from server.";
        emit sig_connection_closed();
    });

    connect(this, &TCPMgr::sig_send_data, this, &TCPMgr::slot_send_data);
}

void TCPMgr::slot_tcp_connect(std::shared_ptr<ServerInfo> si) {

    // 灏濊瘯杩炴帴鍒版湇鍔″櫒
    qDebug() << "Connecting to server...";
    _host = si->_chat_host;
    _port = si->_chat_port;
    _socket.connectToHost(si->_chat_host, _port);
}

void TCPMgr::slot_send_data(ReqId reqId, QByteArray data) {
    if (_socket.state() != QAbstractSocket::ConnectedState) {
        qDebug() << "Socket is not connected. Cannot send data.";
        return;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    quint16 id = reqId;
    quint16 len = data.length();
    out << id << len;
    block.append(data);
    if (_pending) {
        _send_queue.enqueue(block);
        return;
    }
    _pending = true;
    _current_block = block;
    _send_bytes = 0;
    _socket.write(_current_block);
}

void TCPMgr::initHandlers() {
    _handlers.insert(MSG_CHAT_LOGIN_RSP ,[this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug()<< "handle id is "<< id ;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
           qDebug() << "Failed to create QJsonDocument.";
           return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        qDebug()<< "data jsonobj is " << jsonObj ;

        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err ;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;
            emit sig_login_failed(err);
            return;
        }
        auto uid = jsonObj["uid"].toInt();
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto desc = jsonObj["desc"].toString();
        auto user_info = std::make_shared<UserInfo>(uid, name, nick, icon, sex,"",desc);

        UserMgr::instance()->SetUserInfo(user_info);
        UserMgr::instance()->SetToken(jsonObj["token"].toString());

        if(jsonObj.contains("apply_list")){
            UserMgr::instance()->AppendApplyList(jsonObj["apply_list"].toArray());
        }

        //娣诲姞濂藉弸鍒楄〃
        if (jsonObj.contains("friend_list")) {
            UserMgr::instance()->AppendFriendList(jsonObj["friend_list"].toArray());
        }
        emit sig_switch_chatdlg();
    });

    _handlers.insert(ID_SEARCH_USER_REQ, [this](ReqId id, int len, QByteArray data) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
           qDebug() << "Failed to create QJsonDocument.";
           return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err ;
            emit sig_login_failed(err);
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;
            emit sig_login_failed(err);
            return;
        }
        auto search_info = std::make_shared<SearchInfo>(jsonObj["uid"].toInt(),
              jsonObj["name"].toString(), jsonObj["nick"].toString(),
              jsonObj["desc"].toString(), jsonObj["sex"].toInt(), jsonObj["icon"].toString());
        emit sig_user_search(search_info);
    });

    _handlers.insert(ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;

            emit sig_user_search(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Login Failed, err is " << err;
            emit sig_user_search(nullptr);
            return;
        }

         int from_uid = jsonObj["applyuid"].toInt();
         QString name = jsonObj["name"].toString();
         QString desc = jsonObj["desc"].toString();
         QString icon = jsonObj["icon"].toString();
         QString nick = jsonObj["nick"].toString();
         int sex = jsonObj["sex"].toInt();

        auto apply_info = std::make_shared<AddFriendApply>(
                    from_uid, name, desc,
                      icon, nick, sex);

        emit sig_friend_apply(apply_info);
    });

    _handlers.insert(ID_SEARCH_USER_RSP, [this](ReqId id, int len, QByteArray data) {
        qDebug() << "handle id is " << id << " data is " << data;
        // 灏哘ByteArray杞崲涓篞JsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err;

            emit sig_user_search(nullptr);
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Login Failed, err is " << err;
            emit sig_user_search(nullptr);
            return;
        }
       auto search_info =  std::make_shared<SearchInfo>(jsonObj["uid"].toInt(), jsonObj["name"].toString(),
            jsonObj["nick"].toString(), jsonObj["desc"].toString(),
               jsonObj["sex"].toInt(), jsonObj["icon"].toString());

        QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(storage_dir);
        if (!dir.exists() && !dir.mkpath(storage_dir)) {
            qDebug() << "路径错误";
        }else {
            QString head_icon = search_info->_icon;
            QString head_path = QDir(dir.filePath("avatars")).filePath(head_icon);
            QFileInfo headFile(head_path);
            if (!headFile.exists())
            {
                auto download_info = std::make_shared<DownloadInfo>(head_icon, head_path, search_info->_uid);
                TCPFileMgr::instance()->SendDownloadInfo(download_info);
                UserMgr::instance()->AddDownloadFile(search_info->_icon, download_info);
            }
        }
        emit sig_user_search(search_info);
    });

    _handlers.insert(ID_ADD_FRIEND_RSP, [this](ReqId id, int len, QByteArray data) {
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "ID_ADD_FRIEND_RSP Failed to create QJsonDocument.";
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Add Friend Failed, err is Json Parse Err" << err;
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Add Friend Failed, err is " << err;
            return;
        }

         qDebug() << "Add Friend Success " ;
    });

    _handlers.insert(ID_AUTH_FRIEND_RSP, [this](ReqId id, int len, QByteArray data){
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is Json Parse Err" << err;
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }
        auto name = jsonObj["name"].toString();
        auto nick = jsonObj["nick"].toString();
        auto icon = jsonObj["icon"].toString();
        auto sex = jsonObj["sex"].toInt();
        auto uid = jsonObj["uid"].toInt();
        auto rsp = std::make_shared<AuthRsp>(uid, name, nick, icon, sex);
        emit sig_auth_rsp(rsp);

        qDebug() << "Auth Friend Success " ;
    });

    _handlers.insert(ID_NOTIFY_AUTH_FRIEND_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 灏哘ByteArray杞崲涓篞JsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Auth Friend Failed, err is " << err;
            return;
        }

        int from_uid = jsonObj["fromuid"].toInt();
        QString name = jsonObj["name"].toString();
        QString nick = jsonObj["nick"].toString();
        QString icon = jsonObj["icon"].toString();
        int sex = jsonObj["sex"].toInt();

        auto auth_info = std::make_shared<AuthInfo>(from_uid,name,nick, icon, sex);

        emit sig_add_auth_friend(auth_info);
    });

    _handlers.insert(ID_TEXT_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        qDebug() << "handle id is " << id << " data is " << data;
        // 灏哘ByteArray杞崲涓篞JsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Chat Msg Rsp Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Chat Msg Rsp Failed, err is " << err;
            return;
        }

        auto thread_id = jsonObj["thread_id"].toInt();
        auto sender = jsonObj["fromuid"].toInt();
        // the server fans group messages out, so the conversation type comes from the payload
        const bool is_group = jsonObj.value("is_group").toBool();

        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        for (const QJsonValue& data : jsonObj["chat_datas"].toArray()) {
            auto msg_id = data["message_id"].toInt();
            auto unique_id = data["unique_id"].toString();
            auto msg_content = data["content"].toString();
            QString chat_time = data["chat_time"].toString();
            int status = data["status"].toInt();
            auto chat_data = std::make_shared<TextChatData>(msg_id, unique_id, thread_id,
                is_group ? ChatFormType::GROUP : ChatFormType::PRIVATE,
                ChatMsgType::TEXT, msg_content, sender, status, chat_time);
            chat_datas.push_back(chat_data);
        }

        emit sig_chat_msg_rsp(chat_datas);

        qDebug() << "Receive Text Chat Rsp Success " ;
    });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        auto thread_id = jsonObj["thread_id"].toInt();
        auto sender = jsonObj["fromuid"].toInt();
        // the server fans group messages out, so the conversation type comes from the payload
        const bool is_group = jsonObj.value("is_group").toBool();

        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        for (const QJsonValue& data : jsonObj["chat_datas"].toArray()) {
            auto msg_id = data["message_id"].toInt();
            auto unique_id = data["unique_id"].toString();
            auto msg_content = data["content"].toString();
            QString chat_time = data["chat_time"].toString();
            int status = data["status"].toInt();
            auto chat_data = std::make_shared<TextChatData>(msg_id, unique_id, thread_id,
                is_group ? ChatFormType::GROUP : ChatFormType::PRIVATE,
                ChatMsgType::TEXT, msg_content, sender, status, chat_time);
            chat_datas.push_back(chat_data);
        }

        emit sig_text_chat_msg(chat_datas);
    });

    _handlers.insert(ID_NOTIFY_OFF_LINE_REQ,[this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 灏哘ByteArray杞崲涓篞JsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Notify Chat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Notify Chat Msg Failed, err is " << err;
            return;
        }

        auto uid = jsonObj["uid"].toInt();
        qDebug() << "Receive offline Notify Success, uid is " << uid ;
        //鏂紑杩炴帴
        //骞朵笖鍙戦€侀€氱煡鍒扮晫闈?


        emit sig_notify_offline();

    });

    _handlers.insert(ID_HEARTBEAT_RSP,[this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 灏哘ByteArray杞崲涓篞JsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Heart Beat Msg Failed, err is Json Parse Err" << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "Heart Beat Msg Failed, err is " << err;
            return;
        }

        qDebug() << "Receive Heart Beat Msg Success" ;

    });

    _handlers.insert(ID_LOAD_CHAT_THREAD_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 灏哘ByteArray杞崲涓篞JsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "chat thread json parse failed " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "get chat thread rsp failed, error is " << err;
            return;
        }

        qDebug() << "Receive chat thread rsp Success";

        auto thread_array = jsonObj["threads"].toArray();
        std::vector<std::shared_ptr<ChatThreadInfo>> chat_threads;
        for (const QJsonValue& value : thread_array) {
            auto cti = std::make_shared<ChatThreadInfo>();
            cti->_thread_id = value["thread_id"].toInt();
            cti->_type = value["type"].toString();
            cti->_user1_id = value["user1_id"].toInt();
            cti->_user2_id = value["user2_id"].toInt();
            cti->_group_name = value["group_name"].toString();
            for (const QJsonValue& member : value["members"].toArray()) {
                cti->_group_members.push_back(member.toInt());
            }

            // cache the group member profiles so group messages can show nick name and avatar
            for (const QJsonValue& member : value["member_infos"].toArray()) {
                const int member_uid = member["uid"].toInt();
                if (member_uid <= 0) {
                    continue;
                }
                QString member_name = member["nick"].toString();
                if (member_name.isEmpty()) {
                    member_name = member["name"].toString();
                }
                auto member_info = std::make_shared<UserInfo>(member_uid, member_name, member_name,
                    member["icon"].toString(), 0);
                UserMgr::instance()->AddGroupMemberInfo(member_info);
            }
            chat_threads.push_back(cti);
        }

        bool load_more = jsonObj["load_more"].toBool();
        int next_last_id = jsonObj["next_last_id"].toInt();
        //鍙戦€佷俊鍙烽€氱煡鐣岄潰
        emit sig_load_chat_thread(load_more, next_last_id, chat_threads);
    });

    _handlers.insert(ID_CREATE_PRIVATE_CHAT_RSP, [this](ReqId id, int len, QByteArray data) {
       Q_UNUSED(len);
       qDebug() << "handle id is " << id << " data is " << data;
       // 灏哘ByteArray杞崲涓篞JsonDocument
       QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

       // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
       if (jsonDoc.isNull()) {
           qDebug() << "Failed to create QJsonDocument.";
           return;
       }

       QJsonObject jsonObj = jsonDoc.object();

       if (!jsonObj.contains("error")) {
           int err = ErrorCodes::ERR_JSON;
           qDebug() << "parse create private chat json parse failed " << err;
           return;
       }

       int err = jsonObj["error"].toInt();
       if (err != ErrorCodes::SUCCESS) {
           qDebug() << "get create private chat failed, error is " << err;
           return;
       }

       qDebug() << "Receive create private chat rsp Success";

       int uid = jsonObj["uid"].toInt();
       int other_id = jsonObj["other_id"].toInt();
       int thread_id = jsonObj["thread_id"].toInt();

       //鍙戦€佷俊鍙烽€氱煡鐣岄潰
       emit sig_create_private_chat(uid, other_id, thread_id);
       });

    _handlers.insert(ID_LOAD_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        // 灏哘ByteArray杞崲涓篞JsonDocument
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        // 妫€鏌ヨ浆鎹㈡槸鍚︽垚鍔?
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "parse create private chat json parse failed " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "get create private chat failed, error is " << err;
            return;
        }

        qDebug() << "Receive create private chat rsp Success";

        int thread_id = jsonObj["thread_id"].toInt();
        int last_msg_id = jsonObj["last_message_id"].toInt();
        bool load_more = jsonObj["load_more"].toBool();
        bool is_group = jsonObj["is_group"].toBool();

        std::vector<std::shared_ptr<TextChatData>> chat_datas;
        for (const QJsonValue& data : jsonObj["chat_datas"].toArray()) {
            auto send_uid = data["sender"].toInt();
            auto msg_id = data["msg_id"].toInt();
            auto thread_id = data["thread_id"].toInt();
            QString unique_id = data["unique_id"].toString();
            auto msg_content = data["msg_content"].toString();
            QString chat_time = data["chat_time"].toString();
            int status = data["status"].toInt();
            ChatMsgType msg_type = ChatMsgType::TEXT;
            int type_value = data["type"].toInt(-1);
            if (type_value == static_cast<int>(ChatMsgType::PIC)) {
                msg_type = ChatMsgType::PIC;
            } else if (type_value == static_cast<int>(ChatMsgType::FILE)) {
                msg_type = ChatMsgType::FILE;
            } else {
                const QString lower_content = msg_content.toLower();
                if (lower_content.endsWith(".png") || lower_content.endsWith(".jpg") ||
                    lower_content.endsWith(".jpeg") || lower_content.endsWith(".bmp") ||
                    lower_content.endsWith(".gif") || lower_content.endsWith(".webp")) {
                    msg_type = ChatMsgType::PIC;
                } else if (msg_content.startsWith('{') && msg_content.contains("}_")) {
                    msg_type = ChatMsgType::FILE;
                }
            }
            if (is_group) {
                auto chat_data = std::make_shared<TextChatData>(msg_id, unique_id, thread_id, ChatFormType::GROUP,
                msg_type, msg_content, send_uid, status, chat_time);

                chat_datas.push_back(chat_data);
            }else {
                auto chat_data = std::make_shared<TextChatData>(msg_id, unique_id, thread_id, ChatFormType::PRIVATE,
                    msg_type, msg_content, send_uid, status, chat_time);

                chat_datas.push_back(chat_data);
            }
        }

        emit sig_load_chat_msg(thread_id, last_msg_id, load_more, chat_datas);
    });

    _handlers.insert(ID_IMG_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
       Q_UNUSED(len);
       qDebug() << "handle id is " << id << " data is " << data;
       // 将QByteArray转换为QJsonDocument
       QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

       // 检查转换是否成功
       if (jsonDoc.isNull()) {
           qDebug() << "Failed to create QJsonDocument.";
           return;
       }

       QJsonObject jsonObj = jsonDoc.object();

       if (!jsonObj.contains("error")) {
           int err = ErrorCodes::ERR_JSON;
           qDebug() << "parse create private chat json parse failed " << err;
           return;
       }

       int err = jsonObj["error"].toInt();
       if (err != ErrorCodes::SUCCESS) {
           qDebug() << "get create private chat failed, error is " << err;
           return;
       }

       qDebug() << "Receive create private chat rsp Success";

       //收到消息后转发给页面
       auto thread_id = jsonObj["thread_id"].toInt();
       auto unique_id = jsonObj["unique_id"].toString();
       auto unique_name = jsonObj["unique_name"].toString();
        auto md5 = jsonObj["md5"].toString();

       auto sender = jsonObj["fromuid"].toInt();
       auto touid = jsonObj["touid"].toInt();
       auto msg_id = jsonObj["message_id"].toInt();
       QString chat_time = jsonObj["chat_time"].toString();
       int status = jsonObj["status"].toInt();

       auto file_info = UserMgr::instance()->GetTransFileByName(unique_name);
       if (!file_info) {
           qWarning() << "ID_IMG_CHAT_MSG_RSP: trans file not found:" << unique_name;
           return;
       }

       file_info->_msg_id = msg_id;
       file_info->_unique_id = unique_id;
       file_info->_thread_id = thread_id;
       file_info->_from_uid = sender;
       file_info->_to_uid = touid;

       if (thread_id > 0) {
           auto thread = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
           if (thread) {
               thread->ConfirmMsg(unique_id, msg_id, status);
           }
       }

       // auto chat_data = std::make_shared<ImgChatData>(file_info, unique_id, thread_id, ChatFormType::PRIVATE,
       //     ChatMsgType::TEXT, sender, status, chat_time);

       //发送信号通知界面
        //emit sig_chat_img_rsp(thread_id, chat_data);

        QFile file(file_info->_content_or_url);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open file:" << file.errorString();
            return;
        }

        file.seek(0);
        auto buffer = file.read(MAX_FILE_LEN);
        qDebug() << "buffer is " << buffer;

        file_info->_seq = 1;
        file_info->_current_size = static_cast<int>(buffer.size());

        QJsonObject sendObj;
        sendObj["md5"] = md5;
        sendObj["unique_id"] = unique_id;
        sendObj["name"] = file_info->_unique_name;
        sendObj["seq"] = file_info->_seq;
        sendObj["trans_size"] = file_info->_current_size;
        sendObj["total_size"] = file_info->_total_size;
        sendObj["data"] = QString::fromLatin1(buffer.toBase64());
        sendObj["token"] = UserMgr::instance()->GetToken();
        sendObj["uid"] = UserMgr::instance()->GetUid();
        if (file_info->_current_size >= file_info->_total_size) {
            sendObj["last"] = 1;
        }
        else {
            sendObj["last"] = 0;
        }

        QJsonDocument doc_file(sendObj);
       QByteArray fileData = doc_file.toJson(QJsonDocument::Compact);

       //发送消息给ResourceServer
       TCPFileMgr::instance()->SendData(ReqId::ID_IMG_CHAT_UPLOAD_REQ, fileData);
       });

    _handlers.insert(ID_NOTIFY_IMG_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "ID_NOTIFY_IMG_CHAT_MSG_REQ: failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "ID_NOTIFY_IMG_CHAT_MSG_REQ: error is " << err;
            return;
        }

        emit sig_img_chat_msg(jsonObj);
    });

    _handlers.insert(ID_FILE_CHAT_MSG_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if (!jsonObj.contains("error")) {
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "parse create private chat json parse failed " << err;
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "get create private chat failed, error is " << err;
            return;
        }
        auto thread_id = jsonObj["thread_id"].toInt();
        auto unique_id = jsonObj["unique_id"].toString();
        auto unique_name = jsonObj["unique_name"].toString();
        auto md5 = jsonObj["md5"].toString();
        auto total_size = jsonObj["total_size"].toInt();

        auto sender = jsonObj["fromuid"].toInt();
        auto touid = jsonObj["touid"].toInt();
        auto msg_id = jsonObj["message_id"].toInt();
        QString chat_time = jsonObj["chat_time"].toString();
        int status = jsonObj["status"].toInt();

        auto file_info = UserMgr::instance()->GetTransFileByName(unique_name);
       if (!file_info) {
           qWarning() << "ID_IMG_CHAT_MSG_RSP: trans file not found:" << unique_name;
           return;
       }

        file_info->_msg_id = msg_id;
        file_info->_unique_id = unique_id;
        file_info->_thread_id = thread_id;
        file_info->_from_uid = sender;
        file_info->_to_uid = touid;

        if (thread_id > 0) {
            auto thread = UserMgr::instance()->GetChatThreadDataByThreadId(thread_id);
            if (thread) {
                thread->ConfirmMsg(unique_id, msg_id, status);
            }
        }

        QFile file(file_info->_content_or_url);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open file:" << file.errorString();
            return;
        }

        file.seek(0);
        auto buffer = file.read(MAX_FILE_LEN);
        qDebug() << "buffer is " << buffer;

        file_info->_seq = 1;
        file_info->_current_size = static_cast<int>(buffer.size());

        QJsonObject sendObj;
        sendObj["md5"] = md5;
        sendObj["unique_id"] = unique_id;
        sendObj["name"] = file_info->_unique_name;
        sendObj["seq"] = file_info->_seq;
        sendObj["trans_size"] = file_info->_current_size;
        sendObj["total_size"] = file_info->_total_size;
        sendObj["data"] = QString::fromLatin1(buffer.toBase64());
        sendObj["token"] = UserMgr::instance()->GetToken();
        sendObj["uid"] = UserMgr::instance()->GetUid();
        if (file_info->_current_size >= file_info->_total_size) {
            sendObj["last"] = 1;
        }
        else {
            sendObj["last"] = 0;
        }

        QJsonDocument doc_file(sendObj);
        QByteArray fileData = doc_file.toJson(QJsonDocument::Compact);

        //发送消息给ResourceServer
        TCPFileMgr::instance()->SendData(ReqId::ID_FILE_CHAT_UPLOAD_REQ, fileData);
    });

    _handlers.insert(ID_CREATE_GROUP_CHAT_RSP, [this](ReqId id, int len, QByteArray data) {
       Q_UNUSED(len);
       qDebug() << "handle id is " << id << " data is " << data;

       QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

       if (jsonDoc.isNull()) {
           qDebug() << "Failed to create QJsonDocument.";
           return;
       }

       QJsonObject jsonObj = jsonDoc.object();

       if (!jsonObj.contains("error")) {
           int err = ErrorCodes::ERR_JSON;
           qDebug() << "parse create private chat json parse failed " << err;
           return;
       }

       int err = jsonObj["error"].toInt();
       if (err != ErrorCodes::SUCCESS) {
           qDebug() << "get create group chat failed, error is " << err;
           return;
       }

        auto host_uid = UserMgr::instance()->GetUid();
        auto thread_id = jsonObj["thread_id"].toInt();
        QVector<int> members;
        for (const auto& member : jsonObj["members"].toArray()) {
            members.push_back(member.toInt());
        }
        qDebug() << "hello 1046";
        emit sig_add_group_chat_item(members, thread_id);

       });

    _handlers.insert(ID_NOTIFY_CREATE_GROUP_CHAT_RSP, [this](ReqId id, int len, QByteArray data) {
       Q_UNUSED(len);
       qDebug() << "handle id is " << id << " data is " << data;

       QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

       if (jsonDoc.isNull()) {
           qDebug() << "Failed to create QJsonDocument.";
           return;
       }

       QJsonObject jsonObj = jsonDoc.object();

       if (!jsonObj.contains("error")) {
           int err = ErrorCodes::ERR_JSON;
           qDebug() << "parse create private chat json parse failed " << err;
           return;
       }

       int err = jsonObj["error"].toInt();
       if (err != ErrorCodes::SUCCESS) {
           qDebug() << "get create group chat failed, error is " << err;
           return;
       }

        auto host_uid = UserMgr::instance()->GetUid();
        auto thread_id = jsonObj["thread_id"].toInt();
        QVector<int> members;
        for (const auto& member : jsonObj["members"].toArray()) {
            members.push_back(member.toInt());
        }
        qDebug() << "hello 1046";
        emit sig_notify_add_group_chat_item(members, host_uid, thread_id);

       });

}

void TCPMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter =  _handlers.find(id);
    if(find_iter == _handlers.end()){
        qDebug()<< "not found id ["<< id << "] to handle";
        return ;
    }

    find_iter.value()(id,len,data);
}

void TCPMgr::CloseConnection(){
    _socket.close();
}
