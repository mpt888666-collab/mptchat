//
// Created by mpt on 2026/7/14.
//

#ifndef MPTCHAT_ADDUSERITEM_H
#define MPTCHAT_ADDUSERITEM_H

#include "ListItemBase.h"


QT_BEGIN_NAMESPACE

namespace Ui {
    class AddUserItem;
}

QT_END_NAMESPACE

class AddUserItem : public ListItemBase {
    Q_OBJECT

public:
    explicit AddUserItem(QWidget *parent = nullptr);

    ~AddUserItem() override;

    QSize sizeHint() const override {
        return QSize(250, 70); // 返回自定义的尺寸
    }

private:
    Ui::AddUserItem *ui;
};


#endif //MPTCHAT_ADDUSERITEM_H