//
// Created by mpt on 2026/7/25.
//

#ifndef MPTCHAT_CONUSERITEM_H
#define MPTCHAT_CONUSERITEM_H

#include <QWidget>
#include "userdata.h"
#include "ListItemBase.h"


QT_BEGIN_NAMESPACE

namespace Ui {
    class ConUserItem;
}

QT_END_NAMESPACE
class ConUserItem : public ListItemBase {
    Q_OBJECT

public:
    explicit ConUserItem(QWidget *parent = nullptr);
    // void SetInfo(std::shared_ptr<AuthInfo> auth_info);
    // void SetInfo(std::shared_ptr<AuthRsp> auth_rsp);
    void SetInfo(std::shared_ptr<AuthInfo> auth_info);
    void SetInfo(int uid, const QString& name, const QString& icon);
    void ShowRedPoint(bool show = false);
    std::shared_ptr<UserInfo> GetUserInfo() {
        return _info;
    }
    [[nodiscard]] QSize sizeHint() const override;
    ~ConUserItem() override;

    void SetIcon(QString icon);
private:
    Ui::ConUserItem *ui;
    std::shared_ptr<UserInfo> _info;
};


#endif //MPTCHAT_CONUSERITEM_H