//
// Created by mpt on 2026/7/14.
//

#include "StateWidget.h"

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QResizeEvent>

namespace {
// 红点控件的边长（图片自带透明边，实际可见的圆点约 8x8）
constexpr int kRedPointSize = 24;
// 圆点距控件右上角的像素
constexpr int kRedPointInset = 2;
}

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

    _red_point = new QLabel(this);
    _red_point->setObjectName("red_point");
    _red_point->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    _red_point->setFixedSize(kRedPointSize, kRedPointSize);
    _red_point->setVisible(false);
    UpdateRedPointGeometry();
}

void StateWidget::UpdateRedPointGeometry()
{
    if (!_red_point)
    {
        return;
    }

    const int padding = _red_point->width() / 3;
    const int x = width() - kRedPointInset + padding - _red_point->width();
    const int y = kRedPointInset - padding;
    _red_point->move(x, y);
}

void StateWidget::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    UpdateRedPointGeometry();
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

    QWidget::mousePressEvent(event);
}

