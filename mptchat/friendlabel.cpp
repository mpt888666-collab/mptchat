//
// Created by mpt on 2026/7/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FriendLabel.h" resolved

#include "friendlabel.h"

#include <utility>
#include "ui_FriendLabel.h"


FriendLabel::FriendLabel(QWidget *parent) : QFrame(parent), ui(new Ui::FriendLabel) {
    ui->setupUi(this);
    ui->close_label->SetState("normal", "", "",
                                    "selected", "", "");

    connect(ui->close_label, &ClickedLabel::clicked, this, &FriendLabel::slot_close);
}

FriendLabel::~FriendLabel() {
    delete ui;
}

void FriendLabel::SetText(QString text) {
    _text = std::move(text);
    ui->tip_label->setText(_text);
    ui->tip_label->adjustSize();

    QFontMetrics fontMetrics(ui->tip_label->font());
    auto textWidget = fontMetrics.horizontalAdvance(ui->tip_label->text());
    auto textHeight = fontMetrics.height();

    int labelHeight = qMax(textHeight + 6, 28);
    this->setFixedWidth(textWidget + 28 + 10);
    this->setFixedHeight(labelHeight);

    _width = QWidget::width();
    _height = QWidget::height();
}

int FriendLabel::width() const{
    return _width;
}

int FriendLabel::Height() const {
    return _height;
}

QString FriendLabel::Text() const {
    return _text;
}

void FriendLabel::slot_close() {
    emit sig_close(_text);
}