//
// Created by mpt on 2026/7/5.
//

#ifndef MPTCHAT_RESETDIALOG_H
#define MPTCHAT_RESETDIALOG_H

#include <QDialog>
#include "global.h"
#include "HttpMgr.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class ResetDialog;
}

QT_END_NAMESPACE

class ResetDialog : public QDialog {
    Q_OBJECT

public:
    explicit ResetDialog(QWidget *parent = nullptr);

    ~ResetDialog() override;
private slots:
    void slot_reset_mod_finish(ReqId id, QString data, ErrorCodes err);

    void slot_varify_btn_clicked();

    void slot_confirm_btn_clicked();
signals:
    void sigSwitchLogin();
private:
    Ui::ResetDialog *ui;

    QMap<TipErr, QString> _tip_errs;

    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;

    void showTip(QString str, bool b_ok);

    bool checkUserValid();

    bool checkEmailValid();

    bool checkPassValid();

    bool checkVarifyValid();

    void initHandlers();

    void AddTipErr(TipErr te, QString tips);

    void DelTipErr(TipErr te);

};


#endif //MPTCHAT_RESETDIALOG_H