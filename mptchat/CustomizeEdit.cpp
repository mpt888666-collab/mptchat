//
// Created by mpt on 2026/7/12.
//

#include "CustomizeEdit.h"

CustomizeEdit::CustomizeEdit(QWidget *parent) : QLineEdit(parent), _max_len(0) {
    connect(this, &QLineEdit::textChanged,this, &CustomizeEdit::limitTextLength);
}

void CustomizeEdit::limitTextLength(QString text) {
    if (_max_len <= 0) {
        return;
    }
    QByteArray data = text.toUtf8();
    if (data.size() > _max_len) {
        data = data.left(_max_len);
        this->setText(QString::fromUtf8(data));
    }
}

void CustomizeEdit::SetMaxLength(int maxLen) {
    _max_len = maxLen;
}

void CustomizeEdit::focusOutEvent(QFocusEvent *event) {
    qDebug() << "CustomizeEdit focusout";
    QLineEdit::focusOutEvent(event);

    emit sig_focus_out();
}

