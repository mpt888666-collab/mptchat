//
// Created by mpt on 2026/7/24.
//

#ifndef MPTCHAT_FRIENDLABEL_H
#define MPTCHAT_FRIENDLABEL_H

#include <qframe.h>
#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui {
    class FriendLabel;
}

QT_END_NAMESPACE

class FriendLabel : public QFrame {
    Q_OBJECT

public:
    explicit FriendLabel(QWidget *parent = nullptr);

    ~FriendLabel() override;

    void SetText(QString);

    [[nodiscard]] int width() const;

    [[nodiscard]] int Height() const;

    [[nodiscard]] QString Text() const;

private:
    Ui::FriendLabel *ui;

    QString _text;
    int _width;
    int _height;

public slots:
    void slot_close();
signals:
    void sig_close(QString);
};


#endif //MPTCHAT_FRIENDLABEL_H