//
// Created by mpt on 2026/7/12.
//

#include "ChatUserList.h"

#include <qcoreevent.h>
#include <qevent.h>
#include <QScrollBar>
ChatUserList::ChatUserList(QWidget *parent) {
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    this->viewport()->installEventFilter(this);
}

bool ChatUserList::eventFilter(QObject *object, QEvent *event) {
    if (object == viewport()) {
        if (event->type() == QEvent::Enter) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }else if (event->type() == QEvent::Leave) {
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    if (object == viewport() && event->type() == QEvent::Wheel) {
        auto wheelEvent = static_cast<QWheelEvent *>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // 计算滚动步数

        // 设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        // 检查是否滚动到底部
        QScrollBar *scrollBar = this->verticalScrollBar();
        int maxScrollValue = scrollBar->maximum();
        int currentValue = scrollBar->value();
        //int pageSize = 10; // 每页加载的联系人数量

        if (maxScrollValue - currentValue <= 0) {
            // 滚动到底部，加载新的联系人
            qDebug()<<"load more chat user";
            //发送信号通知聊天界面加载更多聊天内容
            emit sig_loading_chat_user();
        }

        return true; // 停止事件传递
    }

    return QListWidget::eventFilter(object, event);
}