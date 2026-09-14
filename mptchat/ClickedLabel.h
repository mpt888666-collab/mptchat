//
// Created by mpt on 2026/7/4.
//

#ifndef MPTCHAT_CLICKEDLABEL_H
#define MPTCHAT_CLICKEDLABEL_H
#include <QLabel>
#include "global.h"
class ClickedLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickedLabel(QWidget* parent);

    void mousePressEvent(QMouseEvent *ev) override;

    // void enterEvent(QEnterEvent *event) override;
    //
    // void leaveEvent(QEvent* event) override;

    void SetState(QString normal="", QString hover="", QString press="",
                  QString select="", QString select_hover="", QString select_press="");

    ClickLbState GetCurState();
    void SetCurState(ClickLbState state);
    void ResetNormalState();
private:
    QString _normal;
    QString _normal_hover;
    QString _normal_press;

    QString _selected;
    QString _selected_hover;
    QString _selected_press;

    ClickLbState _curstate;

signals:
    void clicked(QString, ClickLbState);
};


#endif //MPTCHAT_CLICKEDLABEL_H