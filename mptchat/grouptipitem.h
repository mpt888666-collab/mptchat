//
// Created by mpt on 2026/7/25.
//

#ifndef MPTCHAT_GROUPTIPITEM_H
#define MPTCHAT_GROUPTIPITEM_H

#include <QWidget>

#include "ListItemBase.h"


QT_BEGIN_NAMESPACE

namespace Ui {
    class GroupTipItem;
}

QT_END_NAMESPACE

class GroupTipItem : public ListItemBase {
    Q_OBJECT

public:
    explicit GroupTipItem(QWidget *parent = nullptr);
    [[nodiscard]] QSize sizeHint() const override;
    ~GroupTipItem() override;
    void SetGroupTip(const QString& str);
private:
    Ui::GroupTipItem *ui;
};


#endif //MPTCHAT_GROUPTIPITEM_H