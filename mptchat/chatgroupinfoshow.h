//
// Created by mpt on 2026/9/21.
//

#ifndef MPTCHAT_CHATGROUPINFOSHOW_H
#define MPTCHAT_CHATGROUPINFOSHOW_H

#include <QDialog>


QT_BEGIN_NAMESPACE

namespace Ui {
    class ChatGroupInfoShow;
}

QT_END_NAMESPACE

class ChatGroupInfoShow : public QDialog {
    Q_OBJECT

public:
    explicit ChatGroupInfoShow(QWidget *parent = nullptr);

    ~ChatGroupInfoShow() override;

private:
    Ui::ChatGroupInfoShow *ui;
};


#endif //MPTCHAT_CHATGROUPINFOSHOW_H