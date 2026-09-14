//
// Created by mpt on 2026/7/25.
//

#ifndef MPTCHAT_APPLYFRIENDLIST_H
#define MPTCHAT_APPLYFRIENDLIST_H
#include <QWidget>
#include <QListWidget>

class ApplyFriendList: public QListWidget
{
    Q_OBJECT
public:
    explicit ApplyFriendList(QWidget *parent = nullptr);
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:

signals:
    void sig_show_search(bool);
};


#endif //MPTCHAT_APPLYFRIENDLIST_H