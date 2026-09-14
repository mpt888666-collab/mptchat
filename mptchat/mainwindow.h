//
// Created by mpt on 2026/6/28.
//

#ifndef MPTCHAT_MAINWINDOW_H
#define MPTCHAT_MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include <QStackedWidget>
#include "resetdialog.h"
#include "chatdialog.h"
QT_BEGIN_NAMESPACE

enum UIStatus{
    LOGIN_UI,
    REGISTER_UI,
    RESET_UI,
    CHAT_UI
};

namespace Ui {
    class mainwindow;
}

QT_END_NAMESPACE
class applygroupchat;
class mainwindow : public QMainWindow {
    Q_OBJECT

public:
    explicit mainwindow(QWidget *parent = nullptr);


    ~mainwindow() override;

public slots:
    void on_register();

    void SlotSwitchLogin();

    void SlotSwitchReset();

    void SlotSwitchChat();

    void SlotOffline();

    void offlineLogin();
    void SlotExcepConOffline();

private:
    Ui::mainwindow *ui;
    LoginDialog *_loginDialog;
    RegisterDialog *_registerDialog;
    QStackedWidget * _stack;
    ResetDialog *_resetDialog;
    ChatDialog *_chatDialog;
    UIStatus _ui_status;
};


#endif //MPTCHAT_MAINWINDOW_H