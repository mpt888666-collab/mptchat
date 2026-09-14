//
// Created by mpt on 2026/7/12.
//
#include <QStyleOption>
#include <QPainter>
#include "ListItemBase.h"

ListItemType ListItemBase::GetItemType() {
    return _itemType;
}

void ListItemBase::SetItemType(ListItemType itemType) {
    _itemType = itemType;
}

ListItemBase::ListItemBase(QWidget *parent) {

}

void ListItemBase::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
