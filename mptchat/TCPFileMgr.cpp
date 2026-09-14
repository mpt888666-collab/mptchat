//
// Created by mpt on 2026/8/23.
//

#include "TCPFileMgr.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QtEndian>

#include "UserMgr.h"
#include "TCPMgr.h"

FileTcpThread::FileTcpThread() {
    _thread = new QThread();
    _thread->setObjectName("file_thread");

    auto mgr = TCPFileMgr::instance().get();

    mgr->moveToThread(_thread);
    mgr->socket()->moveToThread(_thread);

    _thread->start();
}

FileTcpThread::~FileTcpThread()
{
    if (!_thread)
        return;

    auto mgr = TCPFileMgr::instance().get();

    if (mgr && _thread->isRunning()) {
        QMetaObject::invokeMethod(
            mgr, "CloseConnection",
            Qt::BlockingQueuedConnection);
    }

    _thread->quit();
    _thread->wait();
    delete _thread;
}

void TCPFileMgr::MoveSocketToThread(QThread* thread) {
    Q_UNUSED(thread)
    // QTcpSocket is asynchronous and safe to keep on the main thread.
}

TCPFileMgr::TCPFileMgr() : QObject(), _b_recv_pending(false), _message_id(0), _message_len(0), _bytes_sent(0), _pending(false){
    connect(&_socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "Connected to resource server!";
        emit sig_con_success(true);
    });

    QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), this, [this](QAbstractSocket::SocketError socketError) {
        Q_UNUSED(socketError)
        qDebug() << "Resource Error:" << _socket.errorString();
    });

    connect(&_socket, &QTcpSocket::disconnected, this, [this]() {
        qDebug() << "Resource Disconnected from server.";
        emit sig_connection_closed();
    });

    connect(&_socket, &QTcpSocket::readyRead, this, [this]() {
        _buffer.append(_socket.readAll());
        while (true) {
            if (!_b_recv_pending) {
                if (_buffer.size() < 6) return;
                auto ptr = _buffer.constData();
                _message_id = qFromBigEndian<qint16>(ptr);
                _message_len = qFromBigEndian<qint32>(ptr + 2);
                _buffer.remove(0, 6);
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

    connect(&_socket, &QTcpSocket::bytesWritten, this, [this](qint64 bytes) {
        _bytes_sent += bytes;
        if (_bytes_sent < _current_block.size()) {
            return;
        }
        if (!_send_queue.isEmpty()) {
            _current_block = _send_queue.dequeue();
            _pending = true;
            _bytes_sent = 0;
            _socket.write(_current_block);
            return;
        }
        _current_block.clear();
        _pending = false;
        _bytes_sent = 0;
    });

    connect(this, &TCPFileMgr::sig_send_data, this, &TCPFileMgr::slot_send_data);


    initHandlers();
}

void TCPFileMgr::SendData(ReqId id, QByteArray data) {
    emit sig_send_data(id, data);
}

void TCPFileMgr::slot_send_data(ReqId reqId, QByteArray data) {
    quint16 id = reqId;
    auto len = static_cast<quint32>(data.length());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::ByteOrder::BigEndian);
    out << id << len;
    block.append(data);
    if (_pending) {
        _send_queue.enqueue(block);
        return;
    }
    _current_block = block;
    _bytes_sent = 0;
    _pending = true;

    _socket.write(_current_block);
}

void TCPFileMgr::slot_tcp_connect(std::shared_ptr<ServerInfo> si) {
    qDebug() << "receive tcp connect signal";
    _host = si->_res_host;
    _port = si->_res_port;
    qDebug() << "Connecting to resource server:" << _host << _port;
    if (_host.isEmpty() || _port == 0) {
        qWarning() << "resource server host/port invalid, check login response res_host/res_port";
    }
    _socket.connectToHost(_host, _port);
}

void TCPFileMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter = _handlers.find(id);
    if (find_iter == _handlers.end()) {
        qDebug() << "not found id [" << id << "] to handle";
        return;
    }

    find_iter.value()(id, len, data);
}

void TCPFileMgr::initHandlers() {
    _handlers.insert(ID_UPLOAD_HEAD_ICON_RSP, [this](ReqId id, quint32 len, QByteArray data) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "TCPFileMgr initHandlers jsonDoc is null";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            qDebug() << "upload rsp missing error";
            return;
        }
        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "upload rsp error: " << err;
            return;
        }

        auto md5 = jsonObj["md5"].toString();
        auto seq = jsonObj["seq"].toInt();
        auto trans_size = jsonObj["trans_size"].toInt();
        auto uid = jsonObj["uid"].toInt();
        auto total_size = jsonObj["total_size"].toInt();
        auto name = jsonObj["name"].toString();
        auto last_seq = jsonObj["last_seq"].toInt();
        qDebug() << "recv upload rsp:" << name << "seq" << seq << "trans_size" << trans_size;

        auto file_info = UserMgr::instance()->GetFileInfoByMD5(md5);
        if (!file_info) {
            qWarning() << "file info not found for md5:" << md5;
            return;
        }

        QFile file(file_info->filePath());
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open file: " << file.errorString();
            return;
        }

        file.seek(trans_size);
        QByteArray buffer = file.read(MAX_FILE_LEN);
        file.close();

        if (buffer.isEmpty()) {
            qDebug() << "upload finished, name:" << name;
            return;
        }

        int next_seq = seq + 1;
        int next_trans_size = trans_size + buffer.size();
        int last = (next_trans_size >= total_size) ? 1 : 0;

        QJsonObject sendObj;
        sendObj["md5"] = md5;
        sendObj["name"] = file_info->fileName();
        sendObj["seq"] = next_seq;
        sendObj["trans_size"] = next_trans_size;
        sendObj["total_size"] = total_size;
        sendObj["last"] = last;
        sendObj["data"] = QString::fromLatin1(buffer.toBase64());
        sendObj["last_seq"] = last_seq;
        sendObj["token"] = UserMgr::instance()->GetToken();
        sendObj["uid"] = uid;

        QJsonDocument doc(sendObj);
        SendData(ID_UPLOAD_HEAD_ICON_REQ, doc.toJson());
    });

    _handlers.insert(ID_IMG_CHAT_UPLOAD_RSP, [this](ReqId id, quint32 len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "img upload rsp json is null";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            qDebug() << "img upload rsp missing error";
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "img upload rsp error:" << err;
            return;
        }

        QString name = jsonObj["name"].toString();
        QString md5 = jsonObj["md5"].toString();
        int seq = jsonObj["seq"].toInt();
        int trans_size = jsonObj["trans_size"].toInt();
        int last = jsonObj["last"].toInt();

        auto file_info = UserMgr::instance()->GetTransFileByName(name);
        if (!file_info) {
            qWarning() << "img upload rsp: trans file not found:" << name;
            return;
        }

        file_info->_seq = seq;
        file_info->_current_size = trans_size;

        auto send_upload_finish = [file_info]() {
            QJsonObject finishObj;
            finishObj["fromuid"] = file_info->_from_uid;
            finishObj["touid"] = file_info->_to_uid;
            finishObj["thread_id"] = file_info->_thread_id;
            finishObj["message_id"] = file_info->_msg_id;
            finishObj["unique_id"] = file_info->_unique_id;
            finishObj["name"] = file_info->_unique_name;
            finishObj["md5"] = file_info->_md5;
            QJsonDocument doc(finishObj);
            emit TCPMgr::instance()->sig_send_data(ReqId::ID_IMG_CHAT_UPLOAD_FINISH_REQ,
                doc.toJson(QJsonDocument::Compact));
        };

        if (last) {
            qDebug() << "img upload finished:" << name;
            send_upload_finish();
            return;
        }

        QFile file(file_info->_content_or_url);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "img upload rsp: could not open file:" << file.errorString();
            return;
        }

        if (!file.seek(trans_size)) {
            qWarning() << "img upload rsp: seek failed:" << trans_size;
            file.close();
            return;
        }

        QByteArray buffer = file.read(MAX_FILE_LEN);
        file.close();
        if (buffer.isEmpty()) {
            qDebug() << "img upload finished (no more data):" << name;
            send_upload_finish();
            return;
        }

        int next_seq = seq + 1;
        int next_trans_size = trans_size + buffer.size();
        int next_last = (next_trans_size >= file_info->_total_size) ? 1 : 0;

        file_info->_seq = next_seq;
        file_info->_current_size = next_trans_size;

        QJsonObject sendObj;
        sendObj["md5"] = md5;
        sendObj["name"] = file_info->_unique_name;
        sendObj["seq"] = next_seq;
        sendObj["trans_size"] = next_trans_size;
        sendObj["total_size"] = file_info->_total_size;
        sendObj["last"] = next_last;
        sendObj["data"] = QString::fromLatin1(buffer.toBase64());
        sendObj["token"] = UserMgr::instance()->GetToken();
        sendObj["uid"] = UserMgr::instance()->GetUid();

        QJsonDocument doc(sendObj);
        SendData(ID_IMG_CHAT_UPLOAD_REQ, doc.toJson());
    });

      _handlers.insert(ID_DOWN_LOAD_FILE_RSP, [this](ReqId id, int len, QByteArray data) {
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
            QString fail_name = jsonObj["name"].toString();
            if (!fail_name.isEmpty()) {
                UserMgr::instance()->RmvDownloadFile(fail_name);
            }
            emit sig_file_download_failed(fail_name, err);
            return;
        }

        qDebug() << "Receive download file info rsp success";

        QString base64Data = jsonObj["data"].toString();
        QString clientPath = jsonObj["client_path"].toString();
        int seq = jsonObj["seq"].toInt();
        bool is_last = jsonObj["is_last"].toBool();
        QString total_size_str = jsonObj["total_size"].toString();
        qint64  total_size = total_size_str.toLongLong(nullptr);
        QString current_size_str = jsonObj["current_size"].toString();
        qint64  current_size = current_size_str.toLongLong(nullptr);
        QString name = jsonObj["name"].toString();

auto file_info = UserMgr::instance()->GetDownloadInfo(name);
        if (file_info == nullptr) {
            qDebug() << "file: " << name << " not found";
            UserMgr::instance()->RmvDownloadFile(name);
            emit sig_file_download_failed(name, ErrorCodes::ERR_NETWORK);
            return;
        }

        qint64 offset = file_info->_current_size;
        file_info->_current_size = current_size;
        file_info->_total_size = total_size;

        QByteArray decodedData = QByteArray::fromBase64(base64Data.toUtf8());

        // make sure the target directory exists (avatar cache, chat file cache, ...)
        const QDir client_dir = QFileInfo(clientPath).absoluteDir();
        if (!client_dir.exists()) {
            client_dir.mkpath(".");
        }

        QFile file(clientPath);

        QIODevice::OpenMode mode;
        if (offset <= 0) {
            mode = QIODevice::WriteOnly;
        }
        else {
            mode = QIODevice::ReadWrite;
        }

        if (!file.open(mode)) {
            qDebug() << "Failed to open file for writing:" << clientPath;
            qDebug() << "Error:" << file.errorString();
            UserMgr::instance()->RmvDownloadFile(name);
            emit sig_file_download_failed(name, ErrorCodes::ERR_NETWORK);
            return;
        }

        if (offset > 0 && !file.seek(offset)) {
            qDebug() << "Failed to seek file to" << offset;
            file.close();
            UserMgr::instance()->RmvDownloadFile(name);
            emit sig_file_download_failed(name, ErrorCodes::ERR_NETWORK);
            return;
        }

        if (!decodedData.isEmpty()) {
            qint64 bytesWritten = file.write(decodedData);
            if (bytesWritten != decodedData.size()) {
                qDebug() << "Failed to write all data. Written:" << bytesWritten
                    << "Expected:" << decodedData.size();
                file.close();
                UserMgr::instance()->RmvDownloadFile(name);
                emit sig_file_download_failed(name, ErrorCodes::ERR_NETWORK);
                return;
            }
        }

        if (is_last || current_size >= total_size) {
            if (total_size > 0) {
                file.resize(total_size);
            }
        }

        file.close();

        emit sig_file_download_progress(name, current_size, total_size);

        if (is_last || current_size >= total_size) {
            qDebug() << "File download completed:" << clientPath;
            UserMgr::instance()->RmvDownloadFile(name);
            emit sig_reset_label_icon(clientPath);
            emit sig_avatar_downloaded(name);
            emit sig_img_chat_downloaded(name, clientPath);
            emit sig_file_download_finished(name, clientPath);
        }
        else {
            file_info->_seq = seq + 1;
            TCPFileMgr::instance()->SendDownloadInfo(file_info);
        }
    });
    _handlers.insert(ID_FILE_CHAT_UPLOAD_RSP, [this](ReqId id, quint32 len, QByteArray data) {
        Q_UNUSED(id);
        Q_UNUSED(len);
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "file upload rsp json is null";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("error")) {
            qDebug() << "file upload rsp missing error";
            return;
        }

        int err = jsonObj["error"].toInt();
        if (err != ErrorCodes::SUCCESS) {
            qDebug() << "file upload rsp error:" << err;
            return;
        }

        QString name = jsonObj["name"].toString();
        QString md5 = jsonObj["md5"].toString();
        int seq = jsonObj["seq"].toInt();
        int trans_size = jsonObj["trans_size"].toInt();
        int last = jsonObj["last"].toInt();

        auto file_info = UserMgr::instance()->GetTransFileByName(name);
        if (!file_info) {
            qWarning() << "file upload rsp: trans file not found:" << name;
            return;
        }

        file_info->_seq = seq;
        file_info->_current_size = trans_size;

        auto send_upload_finish = [file_info]() {
            QJsonObject finishObj;
            finishObj["fromuid"] = file_info->_from_uid;
            finishObj["touid"] = file_info->_to_uid;
            finishObj["thread_id"] = file_info->_thread_id;
            finishObj["message_id"] = file_info->_msg_id;
            finishObj["unique_id"] = file_info->_unique_id;
            finishObj["name"] = file_info->_unique_name;
            finishObj["md5"] = file_info->_md5;
            finishObj["total_size"] = file_info->_total_size;
            QJsonDocument doc(finishObj);
            emit TCPMgr::instance()->sig_send_data(ReqId::ID_FILE_CHAT_UPLOAD_FINISH_REQ,
                doc.toJson(QJsonDocument::Compact));
        };

        if (last) {
            qDebug() << "img upload finished:" << name;
            send_upload_finish();
            return;
        }

        QFile file(file_info->_content_or_url);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "img upload rsp: could not open file:" << file.errorString();
            return;
        }

        if (!file.seek(trans_size)) {
            qWarning() << "img upload rsp: seek failed:" << trans_size;
            file.close();
            return;
        }

        QByteArray buffer = file.read(MAX_FILE_LEN);
        file.close();
        if (buffer.isEmpty()) {
            qDebug() << "img upload finished (no more data):" << name;
            send_upload_finish();
            return;
        }

        int next_seq = seq + 1;
        int next_trans_size = trans_size + buffer.size();
        int next_last = (next_trans_size >= file_info->_total_size) ? 1 : 0;

        file_info->_seq = next_seq;
        file_info->_current_size = next_trans_size;

        QJsonObject sendObj;
        sendObj["md5"] = md5;
        sendObj["name"] = file_info->_unique_name;
        sendObj["seq"] = next_seq;
        sendObj["trans_size"] = next_trans_size;
        sendObj["total_size"] = file_info->_total_size;
        sendObj["last"] = next_last;
        sendObj["data"] = QString::fromLatin1(buffer.toBase64());
        sendObj["token"] = UserMgr::instance()->GetToken();
        sendObj["uid"] = UserMgr::instance()->GetUid();

        QJsonDocument doc(sendObj);
        SendData(ID_FILE_CHAT_UPLOAD_REQ, doc.toJson());
    });
}

void TCPFileMgr::SendDownloadInfo(std::shared_ptr<DownloadInfo> download) {
    if (!download) {
        return;
    }

    QFileInfo local_file(download->_client_path);
    if (local_file.exists() && local_file.size() > 0 && download->_current_size <= 0) {
        download->_current_size = local_file.size();
    }
    if (download->_current_size < 0) {
        download->_current_size = 0;
    }

    QJsonObject jsonObj;
    jsonObj["name"] = download->_name;
    jsonObj["seq"] = download->_seq;
    jsonObj["trans_size"] = download->_current_size;
    jsonObj["total_size"] = download->_total_size;
    jsonObj["token"] = UserMgr::instance()->GetToken();
    jsonObj["uid"] = UserMgr::instance()->GetUid();
    int owner_uid = download->_owner_uid > 0 ? download->_owner_uid : UserMgr::instance()->GetUid();
    jsonObj["owner_uid"] = owner_uid;
    jsonObj["client_path"] = download->_client_path;

    QJsonDocument doc(jsonObj);
    auto send_data = doc.toJson();

    SendData(ID_DOWN_LOAD_FILE_REQ, send_data);
}

void TCPFileMgr::CloseConnection() {
    _socket.close();
}
