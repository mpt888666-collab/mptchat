//
// Created by mpt on 2026/9/8.
//

#include "CheckIndicator.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QEasingCurve>
#include <QVariantAnimation>

CheckIndicator::CheckIndicator(QWidget *parent) : QLabel(parent) {
    setAlignment(Qt::AlignCenter);

    _pop = new QVariantAnimation(this);
    _pop->setStartValue(0.0);
    _pop->setEndValue(1.0);
    _pop->setDuration(200);

    connect(_pop, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const double t = value.toDouble();

        const double spring = QEasingCurve(QEasingCurve::OutBack).valueForProgress(t);
        _scale = 0.8 + 0.2 * spring;

        const double fade = QEasingCurve(QEasingCurve::InOutCubic).valueForProgress(t);
        const double disp = _startDisp + (_targetDisp - _startDisp) * fade;
        _dispChecked = qBound(0.0, disp, 1.0);

        update();
    });
    connect(_pop, &QVariantAnimation::finished, this, [this] {
        _scale = 1.0;
        _dispChecked = _checked ? 1.0 : 0.0;
        update();
    });
}

void CheckIndicator::setChecked(bool checked) {
    if (_checked == checked) {
        return;
    }
    _checked = checked;
    _startDisp = _dispChecked;
    _targetDisp = _checked ? 1.0 : 0.0;
    _pop->stop();
    _pop->start();
}

bool CheckIndicator::isChecked() const {
    return _checked;
}

void CheckIndicator::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const double side = 22.0;
    painter.save();
    painter.translate(width() / 2.0, height() / 2.0);
    painter.scale(_scale, _scale);

    const QRectF disc(-(side - 2.0) / 2.0, -(side - 2.0) / 2.0, side - 2.0, side - 2.0);
    painter.setPen(QPen(QColor("#C8C8C8"), 1.6));
    painter.setBrush(Qt::white);
    painter.drawEllipse(disc);

    if (_dispChecked > 0.001) {
        painter.setOpacity(_dispChecked);

        const QRectF green(-side / 2.0, -side / 2.0, side, side);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor("#07C160"));
        painter.drawEllipse(green);

        QPainterPath path;
        path.moveTo(side * 0.28, side * 0.52);
        path.lineTo(side * 0.45, side * 0.68);
        path.lineTo(side * 0.75, side * 0.32);

        QPen checkPen(Qt::white, 2.4);
        checkPen.setCapStyle(Qt::RoundCap);
        checkPen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(checkPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    painter.restore();
}