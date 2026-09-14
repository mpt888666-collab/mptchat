#ifndef MPTCHAT_CHATVIEW_H
#define MPTCHAT_CHATVIEW_H

#include <QWidget>
#include <QScrollBar>
#include <QList>

class QScrollArea;
class MessageBubble;

QT_BEGIN_NAMESPACE
namespace Ui { class ChatView; }
QT_END_NAMESPACE

class ChatView : public QWidget {
    Q_OBJECT
public:
    explicit ChatView(QWidget *parent = nullptr);
    ~ChatView() override;

    void appendChatItem(QWidget *item);

    void appendMessage(const QString &text, bool isSent,
                       const QString &avatarPath = "",
                       const QString &timestamp = "");

    void clearChatItems();
    void scrollToBottom();

    QList<MessageBubble*> messageBubbles() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::ChatView *ui;
    QScrollBar *m_customBar;
    QScrollArea *m_scrollArea;
    bool _scroll_to_bottom_scheduled = false;
};

#endif //MPTCHAT_CHATVIEW_H
