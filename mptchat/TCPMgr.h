//
// Created by mpt on 2026/7/6.
//

#ifndef MPTCHAT_TCPMGR_H
#define MPTCHAT_TCPMGR_H
#include <QObject>
#include <memory>
#include <QQueue>
#include <QTcpSocket>
#include <QJsonObject>
#include "singleton.h"
#include "global.h"
#include "userdata.h"
#include <QThread>
class TCPThread : public QObject{
    Q_OBJECT
public:
    TCPThread();

    ~TCPThread();

private:
    QThread* _tcp_thread;
};

class TCPMgr : public QObject, public Singleton<TCPMgr>, public std::enable_shared_from_this<TCPMgr>{
    Q_OBJECT
    friend class Singleton<TCPMgr>;
public:
    QTcpSocket* socket() { return &_socket; }
public slots:
    void slot_tcp_connect(std::shared_ptr<ServerInfo> si);

    void slot_send_data(ReqId reqId, QByteArray data);

    void initHandlers();

    void CloseConnection();
signals:
    void sig_connect_success(bool success);

    void sig_send_data(ReqId reqId, QByteArray data);

    void sig_login_failed(int);

    void sig_switch_chatdlg();

    void sig_user_search(std::shared_ptr<SearchInfo>);

    void sig_friend_apply(std::shared_ptr<AddFriendApply>);

    void sig_auth_rsp(std::shared_ptr<AuthRsp>);

    void sig_add_auth_friend(std::shared_ptr<AuthInfo>);

    void sig_text_chat_msg(std::vector<std::shared_ptr<TextChatData>> msglists);

    void sig_chat_msg_rsp(std::vector<std::shared_ptr<TextChatData>> msglists);

    void sig_notify_offline();

    void sig_connection_closed();

    void sig_load_chat_thread(bool load_more, int next_last_id, std::vector<std::shared_ptr<ChatThreadInfo>>);

    void sig_create_private_chat(int, int, int);

    void sig_load_chat_msg(int, int, bool, std::vector<std::shared_ptr<TextChatData>>);

    void sig_img_chat_msg(const QJsonObject &msg);

    void sig_add_group_chat_item(QVector<int> members, int thread_id);

    void sig_notify_add_group_chat_item(QVector<int> members, int host_uid, int thread_id);

    void sig_close_uid();

    void sig_show_red_point(int thread_id);


private:
    QTcpSocket _socket;
    QByteArray _buffer;
    QString _host;
    uint16_t _port;
    bool _b_recv_pending;
    quint16 _message_id;
    quint16 _message_len;

    bool _pending = {false};
    QByteArray _current_block;
    qint64 _send_bytes;
    QQueue<QByteArray> _send_queue;

    QMap<ReqId, std::function<void(ReqId id, int len, QByteArray data)>> _handlers;

    TCPMgr();

    void handleMsg(ReqId id, int len, QByteArray data);
};


#endif //MPTCHAT_TCPMGR_H
