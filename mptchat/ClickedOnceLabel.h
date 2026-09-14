//
// Created by mpt on 2026/7/24.
//

#ifndef MPTCHAT_CLICKEDONCELABEL_H
#define MPTCHAT_CLICKEDONCELABEL_H
#include <QWidget>
#include <QLabel>
class ClickedOnceLabel : public QLabel{
    Q_OBJECT
public:
    explicit ClickedOnceLabel(QWidget * parent = nullptr);

protected:
    void mouseReleaseEvent(QMouseEvent *ev) override;

signals:
    void clicked(QString);
};


#endif //MPTCHAT_CLICKEDONCELABEL_H