//
// Created by mpt on 2026/6/28.
//

// You may need to build the project (run Qt uic code generator) to get "ui_RegisterDialog.h" resolved
#include <QPointer>
#include <utility>
#include "registerdialog.h"
#include "ui_RegisterDialog.h"


RegisterDialog::RegisterDialog(QWidget *parent) : QWidget(parent), ui(new Ui::RegisterDialog), _countdown(5) {
    ui->setupUi(this);
    initHttpHandlers();

    _timer = new QTimer(this);
    ui->stackedWidget->setCurrentIndex(0);

    ui->pass_visible->SetState("unvisible","","","visible",
                            "","");

    ui->confirm_visible->SetState("unvisible","","","visible",
                                    "","");
    ui->err_tip->setProperty("state", "normal");
    repolish(ui->err_tip);

    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->pass_edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->pass_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this]() {
        auto state = ui->confirm_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
        }else{
                ui->confirm_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug() << "Label was clicked!";
    });

    connect(ui->get_btn, &QPushButton::clicked, this, &RegisterDialog::get_bin_clicked);
    connect(ui->confirm_btn, &QPushButton::clicked, this, &RegisterDialog::slot_confirm_btn_clicked);
    auto mgr = HttpMgr::instance().get();
    connect(mgr, &HttpMgr::sig_reg_mod_finish, this, &RegisterDialog::slot_reg_mod_finish);

    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this]() {
        checkUserValid();
    });

    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });

    connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this](){
        checkConfirmValid();
    });

    connect(ui->variety_edit, &QLineEdit::editingFinished, this, [this](){
        checkVarifyValid();
    });

    connect(_timer, &QTimer::timeout, this, [this]() {
        if (_countdown <= 0) {
            _countdown = 5;
            _timer->stop();
            emit sigSwitchLogin();
        }
        --_countdown;
        auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
        ui->tip_label->setText(str);
    });

    connect(ui->cancel_btn, &QPushButton::clicked, this, [this]() {
        _timer->stop();
        emit sigSwitchLogin();
    });

    connect(ui->ReBtn, &QPushButton::clicked, this, [this]() {
        _timer->stop();
        emit sigSwitchLogin();
    });

}

void RegisterDialog::initHttpHandlers() {
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        std::cout << error << std::endl;
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("参数错误"),"err");
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"), "normal");
        qDebug()<< "email is " << email ;
    });

    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj) {
        int error = jsonObj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("注册失败"),"err");
            return;
        }
        showTip(tr("注册成功"), "normal");
        ChangeTipPage();
    });
}

RegisterDialog::~RegisterDialog() {
    delete ui;
}

void RegisterDialog::get_bin_clicked()
{
    // //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
    if (email == "") {
        AddTipErr(TipErr::TIP_EMAIL_EMPTY, tr("邮箱不能为空"));
        return;
    }
    bool match = checkEmailValid();
    if(match){
        //发送http请求获取验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
        HttpMgr::instance()->PostHttpReq(QUrl(gate_url_prefix + "/get_varifycode"),
                     json_obj, ReqId::ID_GET_VARIFY_CODE,Modules::REGISTERMOD);
    }
}

void RegisterDialog::showTip(const QString &str, const QString &key)
{
    ui->err_tip->setText(str);
    ui->err_tip->setProperty("state", key);
    repolish(ui->err_tip);
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),"err");
        return;
    }
    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),"err");
        return;
    }
    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),"err");
        return;
    }
    QJsonObject jsonObj = jsonDoc.object();

    _handlers[id](jsonObj);
}

void RegisterDialog::slot_confirm_btn_clicked() {
    bool valid = checkUserValid();
    if(!valid){
        return;
    }

    valid = checkEmailValid();
    if(!valid){
        return;
    }

    valid = checkPassValid();
    if(!valid){
        return;
    }

    valid = checkVarifyValid();
    if(!valid){
        return;
    }

    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = ui->pass_edit->text();
    json_obj["confirm"] = ui->confirm_edit->text();
    json_obj["variety_code"] = ui->variety_edit->text();
    HttpMgr::instance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                 json_obj, ReqId::ID_REG_USER,Modules::REGISTERMOD);

}

void RegisterDialog::AddTipErr(TipErr te, QString tips) {
    _tip_map[te] = std::move(tips);
    showTip(_tip_map[te], "err");
}

void RegisterDialog::DelTipErr(TipErr te) {
    _tip_map.remove(te);
    if(_tip_map.isEmpty()){
        ui->err_tip->clear();
    }else{
        showTip(_tip_map.first(), "err");
    }
}

bool RegisterDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}


bool RegisterDialog::checkPassValid()
{
    auto pass = ui->pass_edit->text();
    if (pass == "") {
        AddTipErr(TipErr::TIP_PWD_EMPTY, tr("密码不能为空"));
        return false;
    }

    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}



bool RegisterDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
    if (email == "") {
        AddTipErr(TipErr::TIP_EMAIL_EMPTY, tr("邮箱不能为空"));
        return false;
    }
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool RegisterDialog::checkVarifyValid()
{
    auto pass = ui->variety_edit->text();
    if (pass == "") {
        AddTipErr(TipErr::TIP_VARIFY_EMPTY, tr("验证码不能为空"));
        return false;
    }
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

bool RegisterDialog::checkConfirmValid() {
    auto pw = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();
    if (confirm == "") {
        AddTipErr(TipErr::TIP_CONFIRM_EMPTY, tr("确认密码不能为空"));
        return false;
    }
    if (pw != confirm) {
        AddTipErr(TipErr::TIP_PWD_CONFIRM, tr("密码不一致"));
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_CONFIRM);
    return true;
}

void RegisterDialog::ChangeTipPage() {
    _timer->stop();
    ui->stackedWidget->setCurrentIndex(1);
    _timer->start(1000);
}