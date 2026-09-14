//
// Created by mpt on 2026/7/5.
//

// You may need to build the project (run Qt uic code generator) to get "ui_ResetDialog.h" resolved

#include "resetdialog.h"
#include "ui_ResetDialog.h"


ResetDialog::ResetDialog(QWidget *parent) : QDialog(parent), ui(new Ui::ResetDialog) {
    ui->setupUi(this);
    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });

    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });


    connect(ui->variety_edit, &QLineEdit::editingFinished, this, [this](){
         checkVarifyValid();
    });

    initHandlers();

    connect(HttpMgr::instance().get(), &HttpMgr::sig_reset_mod_finish, this,
            &ResetDialog::slot_reset_mod_finish);

    connect(ui->get_btn, &QPushButton::clicked, this, &ResetDialog::slot_varify_btn_clicked);

    connect(ui->cancel_btn, &QPushButton::clicked, this, [this]() {
        emit sigSwitchLogin();
    });

    connect(ui->confirm_btn, &QPushButton::clicked, this, &ResetDialog::slot_confirm_btn_clicked);


}

ResetDialog::~ResetDialog() {
    delete ui;
}

bool ResetDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}


bool ResetDialog::checkPassValid()
{
    auto pass = ui->pass_edit->text();

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

bool ResetDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
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

bool ResetDialog::checkVarifyValid()
{
    auto pass = ui->variety_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

void ResetDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void ResetDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(), false);
}

void ResetDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }

    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void ResetDialog::slot_reset_mod_finish(ReqId id, QString data, ErrorCodes err) {
    qDebug() << "hello here";
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data.toUtf8());
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),false);
        return;
    }
    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;
    }

    _handlers[id](jsonDoc.object());
}

void ResetDialog::initHandlers() {
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](const QJsonObject & jsonObj) {
        int error = jsonObj.value("error").toInt();
        if (error != ErrorCodes::SUCCESS) {
            showTip(tr("参数错误"),false);
           return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"), true);
        qDebug()<< "email is " << email ;
    });

    _handlers.insert(ReqId::ID_RESET_PWD, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("重置成功,点击返回登录"), true);
        qDebug()<< "email is " << email ;
        qDebug()<< "user uuid is " <<  jsonObj["uuid"].toString();
    });
}

void ResetDialog::slot_varify_btn_clicked() {
    auto email = ui->email_edit->text();
    auto check = checkEmailValid();
    if(!check){
        return;
    }

    QJsonObject json_obj;
    json_obj["email"] = email;
    HttpMgr::instance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"),
                                        json_obj, ReqId::ID_GET_VARIFY_CODE,Modules::RESETMOD);
}

void ResetDialog::slot_confirm_btn_clicked() {
    bool valid = checkUserValid();
    if (!valid) {
        return;
    }
    valid = checkEmailValid();
    if (!valid) {
        return;
    }
    valid = checkVarifyValid();
    if (!valid) {
        return;
    }
    valid = checkPassValid();
    if (!valid) {
        return;
    }

    QJsonObject jsonObj;
    jsonObj["email"] = ui->email_edit->text();
    jsonObj["user"] = ui->user_edit->text();
    jsonObj["passwd"] = ui->pass_edit->text();
    jsonObj["variety_code"] = ui->variety_edit->text();
    HttpMgr::instance()->PostHttpReq(gate_url_prefix + "/reset_pwd", jsonObj, ReqId::ID_RESET_PWD, Modules::RESETMOD);
}