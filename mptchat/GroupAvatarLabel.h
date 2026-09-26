//
// Created by mpt on 2026/9/8.
//

#ifndef MPTCHAT_GROUPAVATARLABEL_H
#define MPTCHAT_GROUPAVATARLABEL_H

#include <QLabel>
#include <QPixmap>
#include <QVector>

// WeChat-like group avatar: a rounded square made of member avatars.
class GroupAvatarLabel : public QLabel {
    Q_OBJECT
public:
    explicit GroupAvatarLabel(QWidget *parent = nullptr);

    // Member avatars, drawn in order.
    void setAvatarList(const QVector<QPixmap> &list);
    // Gap between avatars, in pixels (default 2).
    void setSpacing(int px);
    // Show at most the first N avatars (default 9, like WeChat).
    void setMaxShowCount(int cnt);
    void AddRedPoint();
    void ShowRedPoint(bool show=true);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void UpdateRedPointGeometry();
    QVector<QPixmap> m_avatars;
    int m_spacing = 2;
    int m_maxShowCount = 9;
    QLabel * _red_point;
};

#endif //MPTCHAT_GROUPAVATARLABEL_H