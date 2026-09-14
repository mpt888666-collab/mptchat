#include "messagebubble.h"
#include <QFont>
#include <QFontMetrics>
#include <QTextBlock>
#include <QTextDocument>
#include <QPixmap>
#include <QAbstractTextDocumentLayout>

MessageBubble::MessageBubble(ChatRole role, const QString &text, QWidget *parent)
    : ChatItemBase(role, parent)
    , m_pTextEdit(nullptr)
{
    m_pTextEdit = new QTextEdit();
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setFrameShape(QFrame::NoFrame);
    m_pTextEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_pTextEdit->installEventFilter(this);

    QFont font("Microsoft YaHei");
    font.setPointSize(10);
    m_pTextEdit->setFont(font);

    initStyleSheet();
    setWidget(m_pTextEdit);
    setPlainText(text);
}

MessageBubble::MessageBubble(const QString &text, bool isSent,
                             const QString &avatarPath,
                             const QString &timestamp,
                             QWidget *parent)
    : MessageBubble(isSent ? ChatRole::Self : ChatRole::Other, text, parent)
{
    Q_UNUSED(timestamp)
    if (!avatarPath.isEmpty()) {
        QPixmap pix(avatarPath);
        setUserIcon(pix.scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

bool MessageBubble::eventFilter(QObject *o, QEvent *e)
{
    if (m_pTextEdit == o && e->type() == QEvent::Resize) {
        adjustTextHeight();
    }
    return ChatItemBase::eventFilter(o, e);
}

void MessageBubble::setPlainText(const QString &text)
{
    m_pTextEdit->setPlainText(text);

    QTextDocument *doc = m_pTextEdit->document();

    const int CSS_PADDING_V = 20; // 10 + 10
    const int CSS_PADDING_H = 28; // 14 + 14
    const int MAX_BUBBLE_WIDTH = 400;

    // Measure the exact natural unwrapped width
    doc->setTextWidth(100000);
    int naturalWidth = qCeil(doc->idealWidth());

    // Add a small buffer so the last char doesn't wrap due to rounding
    int textWidth = naturalWidth + 6;
    if (textWidth > MAX_BUBBLE_WIDTH) textWidth = MAX_BUBBLE_WIDTH;
    if (textWidth < 20) textWidth = 20;

    // Apply text width so wrapping or single-line decision is made by Qt's layout
    doc->setTextWidth(textWidth);

    // Force re-layout so doc->size() is accurate
    doc->documentLayout()->update();

    int bubbleW = textWidth + CSS_PADDING_H;
    int bubbleH = qCeil(doc->size().height()) + CSS_PADDING_V;
    if (bubbleH < 36) bubbleH = 36;

    m_pTextEdit->setFixedSize(bubbleW, bubbleH);
}

void MessageBubble::adjustTextHeight()
{
    if (!m_pTextEdit) return;
    QTextDocument *doc = m_pTextEdit->document();
    const int CSS_PADDING_V = 20;
    int bubbleH = qCeil(doc->size().height()) + CSS_PADDING_V;
    if (bubbleH < 36) bubbleH = 36;
    m_pTextEdit->setFixedHeight(bubbleH);
}

void MessageBubble::initStyleSheet()
{
    if (m_role == ChatRole::Self) {
        m_pTextEdit->setStyleSheet(
            "QTextEdit{"
            "  background:#95ec69;"
            "  color:#000000;"
            "  padding:10px 14px;"
            "  border:none;"
            "  border-radius:8px;"
            "  border-top-right-radius:2px;"
            "}"
        );
    } else {
        m_pTextEdit->setStyleSheet(
            "QTextEdit{"
            "  background:#ffffff;"
            "  color:#1a1a1a;"
            "  padding:10px 14px;"
            "  border:1px solid #d9d9d9;"
            "  border-radius:8px;"
            "  border-top-left-radius:2px;"
            "}"
        );
    }
}
