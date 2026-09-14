//
// Created by mpt on 2026/7/4.
//

#include "TimerBtn.h"
#include <QTimer>
#include <QMouseEvent>
TimerBtn::TimerBtn(QWidget *parent) : _counter(10){
    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, [this]() {
        --_counter;
        if (_counter <= 0) {
            _timer->stop();
            _counter = 10;
            this->setEnabled(true);
            this->setText("获取");
            return;
        }
        this->setText(QString::number(_counter));
    });
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        qDebug() << "MyButton was released!";
        this->setEnabled(false);
        this->setText(QString::number(_counter));
        _timer->start(1000);
        emit clicked();
    }
    QPushButton::mouseReleaseEvent(e);
}

TimerBtn::~TimerBtn()
{
    _timer->stop();
}

