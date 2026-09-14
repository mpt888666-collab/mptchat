//
// Created by mpt on 2026/9/8.
//

#ifndef MPTCHAT_CHECKINDICATOR_H
#define MPTCHAT_CHECKINDICATOR_H

#include <QLabel>

class QVariantAnimation;

class CheckIndicator : public QLabel {
    Q_OBJECT

public:
    explicit CheckIndicator(QWidget *parent = nullptr);

    void setChecked(bool checked);
    bool isChecked() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool _checked = false;
    double _dispChecked = 0.0;
    double _scale = 1.0;
    double _startDisp = 0.0;
    double _targetDisp = 0.0;
    QVariantAnimation *_pop = nullptr;
};

#endif //MPTCHAT_CHECKINDICATOR_H