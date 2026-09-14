//
// Created by mpt on 2026/6/28.
//

#ifndef MPTCHAT_LOGINDIALOG_H
#define MPTCHAT_LOGINDIALOG_H

#include <QDialog>
#include "ClickedLabel.h"
#include "global.h"
#include "HttpMgr.h"
#include "TCPMgr.h"
QT_BEGIN_NAMESPACE

namespace Ui {
    class LoginDialog;
}

QT_END_NAMESPACE

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);

    ~LoginDialog() override;
signals:
    void switchRegister();

    void switchReset();

    void sig_connect_tcp(std::shared_ptr<ServerInfo> si);
    void sig_connect_res_server(std::shared_ptr<ServerInfo> si);
private slots:
    void slot_forget_pwd();

    void slot_login();

    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);

    void slot_tcp_con_finish(bool success);

    void slot_login_failed(int err);

    void slot_res_tcp_con_finish(bool);

private:
    Ui::LoginDialog *ui;
    std::shared_ptr<ServerInfo> _si;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    QMap<TipErr, QString> _tip_errs;
    int _uid;
    QString _token;

    bool checkUserValid();

    bool checkPwdValid();

    void initHttpHandlers();

    void showTip(QString str, bool b_ok);

    void AddTipErr(TipErr te, QString tips);

    void DelTipErr(TipErr te);
};


#endif //MPTCHAT_LOGINDIALOG_H