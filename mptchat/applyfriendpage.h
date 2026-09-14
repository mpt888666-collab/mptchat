//
// Created by mpt on 2026/7/25.
//

#ifndef MPTCHAT_APPLYFRIENDPAGE_H
#define MPTCHAT_APPLYFRIENDPAGE_H

#include <complex.h>
#include <QWidget>
#include "ApplyFriendItem.h"
#include "userdata.h"


QT_BEGIN_NAMESPACE

namespace Ui {
    class ApplyFriendItem;
    class ApplyFriendPage;
}

QT_END_NAMESPACE

class ApplyFriendPage : public QWidget {
    Q_OBJECT

public:
    explicit ApplyFriendPage(QWidget *parent = nullptr);

    ~ApplyFriendPage() override;

    void AddNewApply(std::shared_ptr<AddFriendApply> apply);

    void loadApplyList();



public slots:
    void slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp);
protected:
    void paintEvent(QPaintEvent *event) override;
private:

    void AddApplyList();

    Ui::ApplyFriendPage *ui;

    std::unordered_map<int, ApplyFriendItem*> _unauth_items;
};


#endif //MPTCHAT_APPLYFRIENDPAGE_H