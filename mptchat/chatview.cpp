#include "chatview.h"
#include "ui_ChatView.h"
#include "messagebubble.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>

ChatView::ChatView(QWidget *parent) : QWidget(parent), ui(new Ui::ChatView) {
    ui->setupUi(this);

    QLayoutItem *oldItem = ui->horizontalLayout->takeAt(0);
    delete oldItem;

    ui->widget->installEventFilter(this);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setWidget(ui->widget);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_customBar = new QScrollBar(Qt::Vertical, this);
    m_customBar->hide();
    m_customBar->setObjectName("chatScrollBar");

    ui->horizontalLayout->addWidget(m_scrollArea, 1);
    ui->horizontalLayout->addWidget(m_customBar, 0);

    ui->m_verticalLayout->addStretch();

    QScrollBar *originBar = m_scrollArea->verticalScrollBar();
    m_customBar->setRange(originBar->minimum(), originBar->maximum());
    m_customBar->setPageStep(originBar->pageStep());

    connect(m_customBar, &QScrollBar::valueChanged, originBar, &QScrollBar::setValue);
    connect(originBar, &QScrollBar::valueChanged, m_customBar, &QScrollBar::setValue);
    connect(originBar, &QScrollBar::rangeChanged, this, [this, originBar](int min, int max) {
        m_customBar->setRange(min, max);
        m_customBar->setPageStep(originBar->pageStep());
    });
}

ChatView::~ChatView() {
    delete ui;
}

void ChatView::appendChatItem(QWidget *item) {
    int spacerIndex = ui->m_verticalLayout->count() - 1;
    ui->m_verticalLayout->insertWidget(spacerIndex, item);
    if (_scroll_to_bottom_scheduled) {
        return;
    }
    _scroll_to_bottom_scheduled = true;
    QTimer::singleShot(50, this, [this]() {
        _scroll_to_bottom_scheduled = false;
        scrollToBottom();
    });
}

void ChatView::scrollToBottom() {
    QScrollBar *bar = m_scrollArea->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void ChatView::clearChatItems() {
    int count = ui->m_verticalLayout->count();
    for (int i = count - 2; i >= 0; --i) {
        QLayoutItem *item = ui->m_verticalLayout->takeAt(i);
        if (item) {
            if (QWidget *w = item->widget()) {
                w->deleteLater();
            }
            delete item;
        }
    }
}

QList<MessageBubble*> ChatView::messageBubbles() const {
    return ui->widget->findChildren<MessageBubble*>();
}
void ChatView::appendMessage(const QString &text, bool isSent,
                             const QString &avatarPath, const QString &timestamp) {
    int spacerIndex = ui->m_verticalLayout->count() - 1;
    auto *bubble = new MessageBubble(text, isSent, avatarPath, timestamp, ui->widget);
    ui->m_verticalLayout->insertWidget(spacerIndex, bubble);

    QTimer::singleShot(0, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum());
    });
}

bool ChatView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->widget) {
        if (event->type() == QEvent::Enter) {
            QScrollBar *bar = m_scrollArea->verticalScrollBar();
            if (bar->maximum() > bar->minimum()) {
                m_customBar->show();
            }
        } else if (event->type() == QEvent::Leave) {
            m_customBar->hide();
        }
    }
    return QWidget::eventFilter(watched, event);
}
