//
// Created by mpt on 2026/7/25.
//

#ifndef MPTCHAT_APPLYFRIENDITEM_H
#define MPTCHAT_APPLYFRIENDITEM_H

#include <QWidget>
#include "userdata.h"

QT_BEGIN_NAMESPACE

namespace Ui {
    class ApplyFriendItem;
}

QT_END_NAMESPACE

class ApplyFriendItem : public QWidget {
    Q_OBJECT

public:
    explicit ApplyFriendItem(QWidget *parent = nullptr);

    void SetInfo(std::shared_ptr<ApplyInfo> apply_info);
    void ShowAddBtn(bool bshow);
    [[nodiscard]] QSize sizeHint() const override {
        return {250, 72};
    }
    int GetUid();

    ~ApplyFriendItem() override;

private:
    Ui::ApplyFriendItem *ui;

    std::shared_ptr<ApplyInfo> _apply_info;
    bool _added;

signals:
    void sig_auth_friend(std::shared_ptr<ApplyInfo> apply_info);
};


#endif //MPTCHAT_APPLYFRIENDITEM_H
