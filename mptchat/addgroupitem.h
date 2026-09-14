//
// Created by mpt on 2026/9/8.
//

#ifndef MPTCHAT_ADDGROUPITEM_H
#define MPTCHAT_ADDGROUPITEM_H

#include <memory>
#include <QWidget>

#include "ListItemBase.h"


class QListWidgetItem;
struct UserInfo;
QT_BEGIN_NAMESPACE

namespace Ui {
    class addgroupitem;
}

QT_END_NAMESPACE
class addgroupitem : public ListItemBase {
    Q_OBJECT

public:
    explicit addgroupitem(QWidget *parent = nullptr);

    ~addgroupitem() override;

    void SetUserInfo(std::shared_ptr<UserInfo> f);

    std::shared_ptr<UserInfo> GetUserInfo() {return _user_info;}

    int GetUid() {
        return _uid;
    }

    [[nodiscard]] QSize sizeHint() const override
    {
        return {300, 60};
    }

    void set_pd_label();

    bool GetSelect() const { return _selected; }

    QString GetName()const {return _name;}

    void CloseLabel();

private:
    Ui::addgroupitem *ui;
    std::shared_ptr<UserInfo> _user_info;
    int _uid;
    int _thread_id;
    QString _icon{};
    QString _name{};
    bool _selected = false;
};


#endif //MPTCHAT_ADDGROUPITEM_H