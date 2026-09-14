//
// Created by mpt on 2026/7/25.
//

// You may need to build the project (run Qt uic code generator) to get "ui_GroupTipItem.h" resolved

#include "grouptipitem.h"
#include "ui_GroupTipItem.h"


GroupTipItem::GroupTipItem(QWidget *parent) : ListItemBase(parent), ui(new Ui::GroupTipItem) {
    ui->setupUi(this);
    SetItemType(ListItemType::GROUP_TIP_ITEM);
}

GroupTipItem::~GroupTipItem() {
    delete ui;
}

QSize GroupTipItem::sizeHint() const {
    return {250, 25};
}

void GroupTipItem::SetGroupTip(const QString& str)
{
    ui->group_label->setText(str);
}
