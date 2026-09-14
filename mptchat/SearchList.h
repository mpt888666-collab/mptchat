//
// Created by mpt on 2026/7/14.
//

#ifndef MPTCHAT_SEARCHLIST_H
#define MPTCHAT_SEARCHLIST_H
#include <qcoreevent.h>
#include <qevent.h>
#include <QWidget>
#include <QListWidget>
#include <QScrollBar>
#include <QAbstractScrollArea>
#include "logindialog.h"
#include "userdata.h"
class SearchList: public QListWidget
{
    Q_OBJECT
public:
    explicit SearchList(QWidget *parent = nullptr);
    void CloseFindDlg();
    void SetSearchEdit(QWidget* edit);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        // 检查事件是否是鼠标悬浮进入或离开
        if (watched == this->viewport()) {
            if (event->type() == QEvent::Enter) {
                // 鼠标悬浮，显示滚动条
                this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            } else if (event->type() == QEvent::Leave) {
                // 鼠标离开，隐藏滚动条
                this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            }
        }

        // 检查事件是否是鼠标滚轮事件
        if (watched == this->viewport() && event->type() == QEvent::Wheel) {
            auto *wheelEvent = dynamic_cast<QWheelEvent*>(event);
            int numDegrees = wheelEvent->angleDelta().y() / 8;
            int numSteps = numDegrees / 15; // 计算滚动步数

            // 设置滚动幅度
            this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

            return true; // 停止事件传递
        }

        return QListWidget::eventFilter(watched, event);
    }
private:
    // void waitPending(bool pending = true);
    bool _send_pending;
    void addTipItem();
    std::shared_ptr<QDialog> _find_dlg;
    QWidget* _search_edit;
    LoginDialog * _loadingDialog;
private slots:
    void slot_item_clicked(QListWidgetItem *item);
    void slot_user_search(std::shared_ptr<SearchInfo> si);
    };


#endif //MPTCHAT_SEARCHLIST_H