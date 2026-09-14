//
// Created by mpt on 2026/7/4.
//

#ifndef MPTCHAT_TIMERBTN_H
#define MPTCHAT_TIMERBTN_H
#include <QPushButton>

class TimerBtn : public QPushButton {
    Q_OBJECT
public:
    explicit TimerBtn(QWidget *parent = nullptr);
    ~TimerBtn() override;

    void mouseReleaseEvent(QMouseEvent *e) override;
private:
    QTimer *_timer;
    int _counter;
};


#endif //MPTCHAT_TIMERBTN_H