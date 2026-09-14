//
// Created by mpt on 2026/7/12.
//

#ifndef MPTCHAT_CUSTOMIZEEDIT_H
#define MPTCHAT_CUSTOMIZEEDIT_H
#include <QWidget>
#include <QObject>
#include <QLineEdit>

class CustomizeEdit : public QLineEdit{
    Q_OBJECT

public:
    CustomizeEdit(QWidget *parent = 0);

    void SetMaxLength(int maxLen);

protected:
    void focusOutEvent(QFocusEvent *event) override;
signals:
    void sig_focus_out();

private slots:
    void limitTextLength(QString text);
private:
    int _max_len;

};


#endif //MPTCHAT_CUSTOMIZEEDIT_H