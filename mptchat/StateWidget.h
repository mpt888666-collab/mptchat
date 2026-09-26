#ifndef MPTCHAT_STATEWIDGET_H
#define MPTCHAT_STATEWIDGET_H
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>

#include "global.h"

class StateWidget : public QLabel {
    Q_OBJECT
public:
    explicit StateWidget(QWidget *parent = nullptr);

    void SetState(QString normal="", QString hover="", QString press="",
                  QString select="", QString select_hover="", QString select_press="");

    ClickLbState GetCurState();
    void ClearState();
    void SetSelected(bool bselected);
    void AddRedPoint();
    void ShowRedPoint(bool show=true);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void clicked();

private:
    void UpdateRedPointGeometry();

    QString _normal;
    QString _normal_hover;
    QString _normal_press;
    QString _selected;
    QString _selected_hover;
    QString _selected_press;
    ClickLbState _curstate;
    QLabel * _red_point;
};

#endif //MPTCHAT_STATEWIDGET_H
