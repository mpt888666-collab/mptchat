//
// Created by mpt on 2026/6/28.
//

#ifndef MPTCHAT_REGISTERDIALOG_H
#define MPTCHAT_REGISTERDIALOG_H

#include <QWidget>
#include "global.h"
#include <QRegularExpression>
#include "HttpMgr.h"
#include <memory>
#include <QPoint>
#include <QTimer>
QT_BEGIN_NAMESPACE

namespace Ui {
    class RegisterDialog;
}

QT_END_NAMESPACE

class RegisterDialog : public QWidget{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);

    ~RegisterDialog() override;
signals:
    void getVariety();

    void sigSwitchLogin();
public slots:
    void get_bin_clicked();

    void slot_reg_mod_finish(ReqId id, QString data, ErrorCodes err);

    void slot_confirm_btn_clicked();
private:
    Ui::RegisterDialog *ui;

    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

    QMap<TipErr, QString> _tip_map;

    QTimer *_timer;

    int _countdown;

    void showTip(const QString &, const QString &);

    void initHttpHandlers();

    bool checkUserValid();

    bool checkEmailValid();

    bool checkPassValid();

    bool checkConfirmValid();

    bool checkVarifyValid();

    void AddTipErr(TipErr te, QString tips);

    void DelTipErr(TipErr te);

    void ChangeTipPage();
};


#endif //MPTCHAT_REGISTERDIALOG_H