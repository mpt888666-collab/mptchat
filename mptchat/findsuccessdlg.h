//
// Created by mpt on 2026/7/20.
//

#ifndef MPTCHAT_FINDSUCCESSDLG_H
#define MPTCHAT_FINDSUCCESSDLG_H

#include <QDialog>
#include "userdata.h"
#include "applyfriend.h"
QT_BEGIN_NAMESPACE

namespace Ui {
    class FindSuccessDlg;
}

QT_END_NAMESPACE

class FindSuccessDlg : public QDialog {
    Q_OBJECT

public:
    explicit FindSuccessDlg(QWidget *parent = nullptr);

    ~FindSuccessDlg() override;

    void SetSearchInfo(std::shared_ptr<SearchInfo> si);


private:
    Ui::FindSuccessDlg *ui;

    std::shared_ptr<SearchInfo> _si;

    QWidget *_parent;
};


#endif //MPTCHAT_FINDSUCCESSDLG_H