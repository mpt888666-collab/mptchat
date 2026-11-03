//
// Created by mpt on 2026/8/24.
//

#ifndef MPTCHAT_USERINFOPAGE_H
#define MPTCHAT_USERINFOPAGE_H

#include <QWidget>

#include "global.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class UserInfoPage;
}

QT_END_NAMESPACE

class UserInfoPage : public QWidget {
    Q_OBJECT

public:
    explicit UserInfoPage(QWidget *parent = nullptr);

    ~UserInfoPage() override;

    // 用 UserMgr 里的当前登录信息填充表单
    void RefreshFromUserMgr();

private:
    QString generateUniqueIconName();
    Ui::UserInfoPage *ui;

public slots:
    void slot_up_btn();

    void slot_submit();

    void slot_update_finish(ReqId id, QString res, ErrorCodes err);
};


#endif //MPTCHAT_USERINFOPAGE_H
