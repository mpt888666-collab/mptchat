//
// Created by mpt on 2026/7/14.
//

#include "StateWidget.h"

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QVBoxLayout>

StateWidget::StateWidget(QWidget *parent): QLabel(parent),_curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor);
    //娣诲姞绾㈢偣
    AddRedPoint();
}

void StateWidget::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    //setProperty("state",normal);
    repolish(this);
}

ClickLbState StateWidget::GetCurState()
{
    return _curstate;
}

void StateWidget::ClearState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state",_normal);
    repolish(this);
    update();
}

void StateWidget::SetSelected(bool bselected)
{
    if(bselected){
        _curstate = ClickLbState::Selected;
        setProperty("state",_selected);
        repolish(this);
        update();
        return;
    }

    _curstate = ClickLbState::Normal;
    setProperty("state",_normal);
    repolish(this);
    update();
    return;
}


void StateWidget::AddRedPoint()
{
    //娣诲姞绾㈢偣绀烘剰鍥?
    _red_point = new QLabel();
    _red_point->setObjectName("red_point");
    auto* layout2 = new QVBoxLayout;
    _red_point->setAlignment(Qt::AlignCenter);
    layout2->addWidget(_red_point);
    layout2->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout2);
    _red_point->setVisible(false);
}

void StateWidget::ShowRedPoint(bool show)
{
    _red_point->setVisible(show);
}

void StateWidget::paintEvent(QPaintEvent *event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    return;
}

void StateWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Selected){
            // 璋冪敤鍩虹被鐨刴ousePressEvent浠ヤ繚璇佹甯哥殑浜嬩欢澶勭悊
            QWidget::mousePressEvent(event);
            return;
        }

        if(_curstate == ClickLbState::Normal){
            _curstate = ClickLbState::Selected;
            setProperty("state",_selected);
            repolish(this);
            emit clicked();
            update();
        }

        return;
    }
    // 璋冪敤鍩虹被鐨刴ousePressEvent浠ヤ繚璇佹甯哥殑浜嬩欢澶勭悊
    QWidget::mousePressEvent(event);
}

