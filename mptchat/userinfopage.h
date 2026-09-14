//
// Created by mpt on 2026/8/24.
//

#ifndef MPTCHAT_USERINFOPAGE_H
#define MPTCHAT_USERINFOPAGE_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui {
    class UserInfoPage;
}

QT_END_NAMESPACE

class UserInfoPage : public QWidget {
    Q_OBJECT

public:
    explicit UserInfoPage(QWidget *parent = nullptr);

    ~UserInfoPage() override;

private:
    QString generateUniqueIconName();
    Ui::UserInfoPage *ui;

public slots:
    void slot_up_btn();
};


#endif //MPTCHAT_USERINFOPAGE_H
