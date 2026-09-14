//
// Created by mpt on 2026/8/28.
//

#include "PictureBubble.h"

#define PIC_MAX_WIDTH 160
#define PIC_MAX_HEIGHT 90

PictureBubble::PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent)
    :ChatItemBase(role, parent)
{
    m_pictureLabel = new QLabel();
    m_pictureLabel->setScaledContents(true);
    setPicture(picture);
    this->setWidget(m_pictureLabel);
}

void PictureBubble::setImageName(const QString &name)
{    m_imageName = name;
}

QString PictureBubble::imageName() const
{
    return m_imageName;
}

void PictureBubble::setPicture(const QPixmap &picture)
{
    if (!m_pictureLabel) {
        return;
    }

    QPixmap pix = picture.scaled(QSize(PIC_MAX_WIDTH, PIC_MAX_HEIGHT),
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (pix.isNull()) {
        pix = QPixmap(PIC_MAX_WIDTH, PIC_MAX_HEIGHT);
        pix.fill(Qt::lightGray);
    }
    m_pictureLabel->setPixmap(pix);
    m_pictureLabel->setFixedSize(pix.size());
}
