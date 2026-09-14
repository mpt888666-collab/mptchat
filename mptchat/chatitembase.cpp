#include "chatitembase.h"
#include <QFont>

// ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent) : QWidget(parent), m_role(role) {
//     m_pNameLabel = new QLabel();
//     m_pNameLabel->setObjectName("chat_user_name");
//     QFont font("Microsoft YaHei");
//     font.setPointSize(9);
//     m_pNameLabel->setFont(font);
//     m_pNameLabel->setFixedHeight(20);
//     m_pNameLabel->setStyleSheet("color:#999999;");
//
//     m_pIconLabel = new QLabel();
//     m_pIconLabel->setScaledContents(true);
//     m_pIconLabel->setFixedSize(42, 42);
//
//     m_pBubble = new QWidget();
//
//     QGridLayout *pGLayout = new QGridLayout();
//     pGLayout->setVerticalSpacing(2);
//     pGLayout->setHorizontalSpacing(8);
//     pGLayout->setContentsMargins(10, 5, 10, 5);
//
//     QSpacerItem *pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
//
//     m_pStatusLabel = new QLabel();
//     m_pStatusLabel->setFixedSize(16, 16);
//     m_pStatusLabel->setScaledContents(true);
//
//     if (m_role == ChatRole::Self) {
//         // Self: avatar on right, spacer on left pushes everything right
//         m_pNameLabel->setContentsMargins(0, 0, 8, 0);
//         m_pNameLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//         pGLayout->addWidget(m_pNameLabel, 0, 2, 1, 1);
//         pGLayout->addWidget(m_pIconLabel, 0, 3, 2, 1, Qt::AlignTop);
//         pGLayout->addItem(pSpacer, 1, 0, 1, 1);
//         pGLayout->addWidget(m_pStatusLabel, 1, 1, 1, 1, Qt::AlignCenter);
//         pGLayout->addWidget(m_pBubble, 1, 2, 1, 1);
//         pGLayout->setColumnStretch(0, 2);
//         pGLayout->setColumnStretch(1, 0);
//         pGLayout->setColumnStretch(2, 3);
//         pGLayout->setColumnStretch(3, 0);
//     } else {
//         // Other: avatar on left, spacer on right
//         m_pNameLabel->setContentsMargins(8, 0, 0, 0);
//         m_pNameLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
//         pGLayout->addWidget(m_pIconLabel, 0, 0, 2, 1, Qt::AlignTop);
//         pGLayout->addWidget(m_pNameLabel, 0, 1, 1, 1);
//         pGLayout->addWidget(m_pBubble, 1, 1, 1, 1);
//         pGLayout->addItem(pSpacer, 1, 2, 1, 1);
//         pGLayout->setColumnStretch(0, 0);
//         pGLayout->setColumnStretch(1, 3);
//         pGLayout->setColumnStretch(2, 2);
//     }
//
//     this->setLayout(pGLayout);
// }

ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent)
    : QWidget(parent)
    , m_role(role)
{
    m_pNameLabel    = new QLabel();
    m_pNameLabel->setObjectName("chat_user_name");
    QFont font("Microsoft YaHei");
    font.setPointSize(9);
    m_pNameLabel->setFont(font);
    m_pNameLabel->setFixedHeight(20);
    m_pIconLabel    = new QLabel();
    m_pIconLabel->setScaledContents(true);
    m_pIconLabel->setFixedSize(42, 42);
    m_pBubble       = new QWidget();
    QGridLayout *pGLayout = new QGridLayout();
    pGLayout->setVerticalSpacing(3);
    pGLayout->setHorizontalSpacing(3);
    //pGLayout->setMargin(3);
    QSpacerItem*pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    //添加状态图标控件
    m_pStatusLabel = new QLabel();
    m_pStatusLabel->setFixedSize(16, 16);
    m_pStatusLabel->setScaledContents(true);

    if(m_role == ChatRole::Self)
    {
        m_pNameLabel->setContentsMargins(0,0,8,0);
        m_pNameLabel->setAlignment(Qt::AlignRight);
        //名字标签
        pGLayout->addWidget(m_pNameLabel, 0,2, 1,1);
        //icon 头像
        pGLayout->addWidget(m_pIconLabel, 0, 3, 2,1, Qt::AlignTop);
        //第 0 列：依然是 pSpacer，占用第 1 行，第 0 列
        pGLayout->addItem(pSpacer, 1, 0, 1, 1);
        //气泡控件
        pGLayout->addWidget(m_pBubble, 1,2, 1,1);
        //状态图标
        pGLayout->addWidget(m_pStatusLabel, 1, 1, 1, 1, Qt::AlignCenter);
        pGLayout->setColumnStretch(0, 2);
        pGLayout->setColumnStretch(1, 0);  // status 图标 (固定大小)
        pGLayout->setColumnStretch(2, 3);  // 名字 + 气泡 (主要拉伸区域)
        pGLayout->setColumnStretch(3, 0);  // 头像 (固定大小)
    }else{
        m_pNameLabel->setContentsMargins(8,0,0,0);
        m_pNameLabel->setAlignment(Qt::AlignLeft);
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2,1, Qt::AlignTop);
        pGLayout->addWidget(m_pNameLabel, 0,1, 1,1);
        pGLayout->addWidget(m_pBubble, 1,1, 1,1);
        pGLayout->addItem(pSpacer, 2, 2, 1, 1);
        pGLayout->setColumnStretch(1, 3);
        pGLayout->setColumnStretch(2, 2);
    }
    this->setLayout(pGLayout);
}

void ChatItemBase::setUserName(const QString &name) { m_pNameLabel->setText(name); }
void ChatItemBase::setUserIcon(const QPixmap &icon) { m_pIconLabel->setPixmap(icon); }

void ChatItemBase::setWidget(QWidget *w) {
    QGridLayout *pGLayout = qobject_cast<QGridLayout *>(this->layout());
    if (!pGLayout) return;
    if (m_pBubble) {
        pGLayout->replaceWidget(m_pBubble, w);
        delete m_pBubble;
    }
    m_pBubble = w;
}

void ChatItemBase::setStatus(int status) {
    if (status == (int)MsgStatus::UN_READ) {
        m_pStatusLabel->setStyleSheet("background:#ff3b30; border-radius:8px;");
        return;
    }
    if (status == (int)MsgStatus::SEND_FAILED) {
        m_pStatusLabel->setText(QStringLiteral("!"));
        m_pStatusLabel->setAlignment(Qt::AlignCenter);
        m_pStatusLabel->setStyleSheet("color:#ff3b30; font-weight:bold; font-size:12px; border-radius:8px; border:1px solid #ff3b30;");
        return;
    }
    if (status == (int)MsgStatus::READED) {
        m_pStatusLabel->setStyleSheet("background:#07c160; border-radius:8px;");
        return;
    }
}
