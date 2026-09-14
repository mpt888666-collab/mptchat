//
// Created by mpt on 2026/6/28.
//

#include "mainwindow.h"

#include <QDir>
#include <QPushButton>

#include "ui_mainwindow.h"
#include "TCPMgr.h"
#include <QMessageBox>
#include <QStandardPaths>

#include "usermgr.h"
#include "applygroupchat.h"
#include "addgroupitem.h"
mainwindow::mainwindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::mainwindow) {
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/res/app.ico"));

    _loginDialog = new LoginDialog();
    _registerDialog = new RegisterDialog();
    _resetDialog = new ResetDialog();
    _chatDialog = new ChatDialog();
    _stack = new QStackedWidget(this);

    _stack->addWidget(_loginDialog);
    _stack->addWidget(_registerDialog);
    _stack->addWidget(_resetDialog);
    setCentralWidget(_stack);

    _ui_status = LOGIN_UI;

    connect(_loginDialog, &LoginDialog::switchRegister, this, &mainwindow::on_register);
    connect(_registerDialog, &RegisterDialog::sigSwitchLogin, this, &mainwindow::SlotSwitchLogin);
    connect(_loginDialog, &LoginDialog::switchReset, this, &mainwindow::SlotSwitchReset);
    connect(_resetDialog, &ResetDialog::sigSwitchLogin, this, &mainwindow::SlotSwitchLogin);

    connect(TCPMgr::instance().get(),&TCPMgr::sig_switch_chatdlg, this, &mainwindow::SlotSwitchChat);
    connect(TCPMgr::instance().get(),&TCPMgr::sig_notify_offline, this, &mainwindow::SlotOffline);
    connect(TCPMgr::instance().get(),&TCPMgr::sig_connection_closed, this, &mainwindow::SlotExcepConOffline);
}

void mainwindow::SlotExcepConOffline()
{
    // 使用静态方法直接弹出一个信息框
    QMessageBox::information(this, "下线提示", "心跳超时或临界异常，该终端下线！");
    TCPMgr::instance()->CloseConnection();
    offlineLogin();
}

mainwindow::~mainwindow() {
    delete ui;
}

void mainwindow::on_register() {
    _ui_status = REGISTER_UI;
    _stack->setCurrentIndex(1);
}

void mainwindow::SlotSwitchLogin() {
    _ui_status = LOGIN_UI;
    _stack->setCurrentIndex(0);
}

void mainwindow::SlotSwitchReset() {
    _ui_status = RESET_UI;
    _stack->setCurrentIndex(2);
}

void mainwindow::SlotSwitchChat() {
    _ui_status = CHAT_UI;
    _chatDialog->show();
    _chatDialog->setMinimumSize(QSize(1050,900));
    _chatDialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    this->hide();
    _chatDialog->loadChatList();

}

void mainwindow::SlotOffline() {
    QMessageBox::information(this, "下线提示", "同账号异地登录，该终端下线！");
    TCPMgr::instance()->CloseConnection();
    offlineLogin();
}

void mainwindow::offlineLogin(){
    if(_ui_status == LOGIN_UI){
        return;
    }

    // Close chat dialog completely
    _chatDialog->hide();

    // Re-create login dialog and put it back into the stack
    if (_loginDialog) {
        _stack->removeWidget(_loginDialog);
        delete _loginDialog;
    }
    _loginDialog = new LoginDialog();
    _stack->insertWidget(0, _loginDialog);

    // Re-connect signals
    connect(_loginDialog, &LoginDialog::switchRegister, this, &mainwindow::on_register);
    connect(_loginDialog, &LoginDialog::switchReset, this, &mainwindow::SlotSwitchReset);

    // Show main window and switch to login page
    _stack->setCurrentIndex(0);
    this->setMaximumSize(300, 500);
    this->setMinimumSize(300, 500);
    this->resize(300, 500);
    this->show();
    _ui_status = LOGIN_UI;

    qDebug() << "relogic";
}

