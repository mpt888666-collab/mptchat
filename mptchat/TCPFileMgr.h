//
// Created by mpt on 2026/8/23.
//

#ifndef MPTCHAT_TCPFILEMGR_H
#define MPTCHAT_TCPFILEMGR_H
#include <complex.h>
#include <QObject>
#include <QTcpSocket>
#include "singleton.h"
#include "global.h"
#include <QQueue>

#include "userdata.h"

class FileTcpThread {
public:
    FileTcpThread();
    ~FileTcpThread();

private:
    QThread* _thread;
};
class TCPFileMgr : public QObject, public Singleton<TCPFileMgr>, std::enable_shared_from_this<TCPFileMgr>{
    Q_OBJECT
    friend class Singleton<TCPFileMgr>;
public:
    QTcpSocket* socket() { return &_socket; }

    void MoveSocketToThread(QThread* thread);

    void SendData(ReqId, QByteArray);
private:
    TCPFileMgr();
    QTcpSocket _socket;
    QString _host;
    quint16 _port{};
    QByteArray _buffer;
    quint16 _message_id;
    quint32 _message_len;
    bool _b_recv_pending;
    bool _pending;
    QByteArray  _current_block;
    qint64        _bytes_sent;
    QMap<int, std::function<void(ReqId, quint32, QByteArray)>> _handlers;
    QQueue<QByteArray> _send_queue;
signals:
    void sig_con_success(bool);
    void sig_connection_closed();
    void sig_send_data(ReqId, QByteArray);
    void sig_user_info_page_show_icon();
    void sig_chatDialog_head_icon();
    void sig_reset_label_icon(QString);
    void sig_img_chat_downloaded(QString name, QString local_path);
    void sig_file_download_progress(QString name, qint64 current, qint64 total);
    void sig_file_download_finished(QString name, QString local_path);
    void sig_file_download_failed(QString name, int error);
    // an avatar image finished downloading and is now cached locally
    void sig_avatar_downloaded(QString name);

    void sig_close_uid();

public slots:
    void slot_send_data(ReqId, QByteArray);
    void slot_tcp_connect(std::shared_ptr<ServerInfo> si);

    void handleMsg(ReqId id, int len, QByteArray data);

    void initHandlers();

    void SendDownloadInfo(std::shared_ptr<DownloadInfo> download);

    void CloseConnection();
};


#endif //MPTCHAT_TCPFILEMGR_H
