//
// Created by mpt on 2026/9/8.
//

#ifndef MPTCHAT_APPLYGROUPCHAT_H
#define MPTCHAT_APPLYGROUPCHAT_H

#include <QDialog>
#include <QMap>

class QListWidgetItem;
QT_BEGIN_NAMESPACE

namespace Ui {
    class applygroupchat;
}

QT_END_NAMESPACE
class addgroupitem;

class applygroupchat : public QDialog {
    Q_OBJECT

public:
    explicit applygroupchat(QWidget *parent = nullptr);

    ~applygroupchat() override;

    void addItem(addgroupitem* item);

    void clearItems();

    void clearItemsGroup();

public slots:
    void slot_cancel_btn();

    void slot_add_group(QListWidgetItem* item);

    void slot_send_add_group_request();
private:
    Ui::applygroupchat *ui;

    QMap<int, addgroupitem*> _friend_list_map;
    QMap<int, addgroupitem*> _group_friend_list_map;
};


#endif //MPTCHAT_APPLYGROUPCHAT_H
