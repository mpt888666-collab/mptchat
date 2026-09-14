#ifndef MPTCHAT_MESSAGEBUBBLE_H
#define MPTCHAT_MESSAGEBUBBLE_H

#include "chatitembase.h"
#include <QTextEdit>

class MessageBubble : public ChatItemBase
{
    Q_OBJECT
public:
    explicit MessageBubble(ChatRole role, const QString &text, QWidget *parent = nullptr);

    explicit MessageBubble(const QString &text, bool isSent,
                           const QString &avatarPath,
                           const QString &timestamp,
                           QWidget *parent = nullptr);

    void setPlainText(const QString &text);

protected:
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void adjustTextHeight();
    void initStyleSheet();
    QTextEdit *m_pTextEdit;
};

#endif
