//
// Created by mpt on 2026/8/28.
//

#ifndef MPTCHAT_PICTUREBUBBLE_H
#define MPTCHAT_PICTUREBUBBLE_H
#include "chatitembase.h"
#include <QObject>
class PictureBubble : public ChatItemBase{
    Q_OBJECT
public:
    PictureBubble(const QPixmap &picture, ChatRole role, QWidget *parent = nullptr);
    void setImageName(const QString &name);
    QString imageName() const;
    void setPicture(const QPixmap &picture);

private:
    QString m_imageName;
    QLabel *m_pictureLabel = nullptr;
};


#endif //MPTCHAT_PICTUREBUBBLE_H
