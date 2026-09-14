//
// Created by mpt on 2026/7/24.
//

#include "ClickedOnceLabel.h"
#include <QEvent>
#include <QMouseEvent>
ClickedOnceLabel::ClickedOnceLabel(QWidget *parent):QLabel(parent)
{
    setCursor(Qt::PointingHandCursor);
}

void ClickedOnceLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(this->text());
        return;
    }
    // 调用基类的mousePressEvent以保证正常的事件处理
    QLabel::mousePressEvent(event);
}