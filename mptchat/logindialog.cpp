//
// Created by mpt on 2026/6/28.
//

// You may need to build the project (run Qt uic code generator) to get "ui_LoginDialog.h" resolved

#include "logindialog.h"
#include "ui_LoginDialog.h"
#include <QKeyEvent>

#include "TCPFileMgr.h"

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent), ui(new Ui::LoginDialog) {
    ui->setupUi(this);
    initHttpHandlers();
    ui->tx_label->setPixmap(QPixmap(":/images/res/tx.jpg"));

    ui->pass_visible->SetState("unvisible","","","visible",
                            "","");

    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->pass_edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->pass_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });
    connect(ui->register_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);

    connect(ui->login_btn, &QPushButton::clicked, this, &LoginDialog::slot_login);

    connect(HttpMgr::instance().get(), &HttpMgr::sig_login_mod_finish, this,
            &LoginDialog::slot_login_mod_finish);

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this]() {
        checkPwdValid();
    });
    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this]() {
        checkUserValid();
    });

    connect(this, &LoginDialog::sig_connect_tcp, TCPMgr::instance().get(), &TCPMgr::slot_tcp_connect);
    connect(TCPMgr::instance().get(), &TCPMgr::sig_connect_success, this, &LoginDialog::slot_tcp_con_finish);
    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_con_success, this, &LoginDialog::slot_res_tcp_con_finish);
    connect(this, &LoginDialog::sig_connect_res_server,TCPFileMgr::instance().get(),&TCPFileMgr::slot_tcp_connect);

    connect(TCPMgr::instance().get(), &TCPMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);


}

LoginDialog::~LoginDialog() {
    delete ui;
}

void LoginDialog::slot_forget_pwd() {
    emit switchReset();
}

void LoginDialog::slot_login() {
    qDebug()<<"login btn clicked";
    if(checkUserValid() == false){
        return;
    }

    if(checkPwdValid() == false){
        return ;
    }

    auto user = ui->user_edit->text();
    auto pwd = ui->pass_edit->text();
    auto email = ui->email_edit->text();
    //鍙戦€乭ttp璇锋眰鐧诲綍
    QJsonObject json_obj;
    json_obj["user"] = user;
    json_obj["passwd"] = pwd;
    json_obj["email"] = email;
    HttpMgr::instance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER,Modules::LOGINMOD);
}

bool LoginDialog::checkUserValid(){

    auto user = ui->user_edit->text();
    if(user.isEmpty()){
        qDebug() << "User empty " ;
        AddTipErr(TipErr::TIP_USER_EMPTY, tr("用户名不能为空"));
        return false;
    }

    return true;
}

bool LoginDialog::checkPwdValid(){
    auto pwd = ui->pass_edit->text();
    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码大小必须为6~15"));
        return false;
    }

    return true;
}

void LoginDialog::initHttpHandlers() {
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("登录失败"),false);
            return;
        }

        auto user = jsonObj["user"].toString();

        _si = std::make_shared<ServerInfo>();
        _si->_uid = jsonObj["uid"].toInt();
        _si->_chat_host = jsonObj["chat_host"].toString();
        _si->_chat_port = jsonObj["chat_port"].toString().toUShort();
        _si->_res_host = jsonObj["res_host"].toString();
        _si->_res_port = jsonObj["res_port"].toString().toUShort();
        _si->_token = jsonObj["token"].toString();

        _uid = _si->_uid;
        _token = _si->_token;
        showTip(tr("登录成功"), true);
        qDebug()<< "user is " << user << " uid is " << _si->_uid <<" host is "
                 << _si->_chat_host << " Port is " << _si->_chat_port << " Token is " << _si->_token;
        emit sig_connect_tcp(_si);
   });
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err) {
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("登录失败"),false);
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败"),false);
        return;
    }

    if(!jsonDoc.isObject()){
        showTip(tr("json解析失败"),false);
        return;
    }
    qDebug() << "this id is " << id;

    _handlers[id](jsonDoc.object());

    return;
}

void LoginDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void LoginDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(), false);
}

void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }

    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{

    if(bsuccess){
        showTip(tr("鑱婂ぉ鏈嶅姟杩炴帴鎴愬姛锛屾鍦ㄨ繛鎺ヨ祫婧愭湇鍔″櫒..."),true);
        emit sig_connect_res_server(_si);

    }else{
        showTip(tr("缃戠粶寮傚父"),false);
        //enableBtn(true);
    }

}
void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("鐧诲綍澶辫触, err is %1")
                             .arg(err);
    showTip(result,false);
    // enableBtn(true);
}

void LoginDialog::slot_res_tcp_con_finish(bool bsuccess) {
    if (!bsuccess) {
        showTip("资源服务器连接失败",false);
        return;
    }
    showTip("资源服务器连接成功，正在登陆...", true);
    QJsonObject jsonObj;
    jsonObj["uid"] = _si->_uid;
    jsonObj["token"] = _si->_token;

    QJsonDocument jsonDoc = QJsonDocument(jsonObj);
    QByteArray data = jsonDoc.toJson(QJsonDocument::Indented);

    emit TCPMgr::instance()->sig_send_data(ReqId::ID_CHAT_LOGIN, data);

}

