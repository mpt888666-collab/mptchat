#ifndef MPTCHAT_CHATITEMBASE_H
#define MPTCHAT_CHATITEMBASE_H
#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QSpacerItem>
#include "global.h"
class ChatItemBase : public QWidget {
    Q_OBJECT
public:
    explicit ChatItemBase(ChatRole role, QWidget *parent = nullptr);
    void setUserName(const QString &name);
    void setUserIcon(const QPixmap &icon);
    void setWidget(QWidget *w);
    void setStatus(int status);
    ChatRole role() const { return m_role; }
protected:
    ChatRole m_role;
    QLabel *m_pNameLabel;
    QLabel *m_pIconLabel;
    QWidget *m_pBubble;
    QLabel *m_pStatusLabel;
};
#endif