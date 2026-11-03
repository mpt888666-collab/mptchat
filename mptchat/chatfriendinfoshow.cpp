//
// Created by mpt on 2026/9/21.
//

#include "chatfriendinfoshow.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include "UserMgr.h"

namespace {

QPixmap makeAvatar(const QString &icon, const QString &name, int size)
{
    QPixmap out(size, size);
    out.fill(Qt::transparent);

    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath clip;
    clip.addEllipse(0, 0, size, size);
    painter.setClipPath(clip);

    QPixmap src;
    if (!icon.isEmpty()) {
        src = QPixmap(UserMgr::GetAvatarLocalPath(icon));
    }

    if (!src.isNull()) {
        painter.drawPixmap(0, 0, src.scaled(size, size,
            Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        return out;
    }

    const int hue = (qHash(name) % 360 + 360) % 360;
    painter.fillRect(0, 0, size, size, QColor::fromHsv(hue, 90, 224));

    QFont font = painter.font();
    font.setPixelSize(size / 2);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter,
                     name.isEmpty() ? QStringLiteral("?") : name.left(1).toUpper());
    return out;
}

QFrame *makeDivider(QWidget *parent)
{
    auto *line = new QFrame(parent);
    line->setObjectName("infoDivider");
    line->setFixedHeight(1);
    return line;
}

} // namespace

ChatFriendInfoShow::ChatFriendInfoShow(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("会话信息"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(300, 420);
    setStyleSheet(R"(
        QDialog { background: #ffffff; }
        QLabel#infoName { color: #111111; font-size: 18px; font-weight: 600; }
        QLabel#infoUid { color: #9a9a9a; font-size: 12px; }
        QFrame#infoDivider { background: #ececec; border: none; }
        QPushButton#infoAction {
            border: none;
            background: transparent;
            text-align: left;
            padding-left: 18px;
            font-size: 14px;
            color: #222222;
        }
        QPushButton#infoAction:hover { background: #f5f6f8; }
        QPushButton#infoAction:pressed { background: #eceef1; }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *header = new QWidget(this);
    auto *header_layout = new QVBoxLayout(header);
    header_layout->setContentsMargins(0, 30, 0, 18);
    header_layout->setSpacing(6);

    _avatar_label = new QLabel(header);
    _avatar_label->setFixedSize(80, 80);
    _avatar_label->setAlignment(Qt::AlignCenter);

    _name_label = new QLabel(header);
    _name_label->setObjectName("infoName");
    _name_label->setAlignment(Qt::AlignCenter);

    _uid_label = new QLabel(header);
    _uid_label->setObjectName("infoUid");
    _uid_label->setAlignment(Qt::AlignCenter);

    header_layout->addWidget(_avatar_label, 0, Qt::AlignHCenter);
    header_layout->addWidget(_name_label);
    header_layout->addWidget(_uid_label);
    root->addWidget(header);

    root->addWidget(makeDivider(this));

    auto *clear_btn = new QPushButton(QStringLiteral("清空聊天记录"), this);
    clear_btn->setObjectName("infoAction");
    clear_btn->setFixedHeight(52);
    clear_btn->setCursor(Qt::PointingHandCursor);
    connect(clear_btn, &QPushButton::clicked, this, &ChatFriendInfoShow::sig_clear_chat_requested);
    root->addWidget(clear_btn);

    root->addWidget(makeDivider(this));
    root->addStretch();
}

ChatFriendInfoShow::~ChatFriendInfoShow() = default;

void ChatFriendInfoShow::SetPrivateChatInfo(const QString &name, const QString &icon, int uid)
{
    const QString shown_name = name.isEmpty() ? QStringLiteral("用户") : name;

    _name_label->setText(shown_name);
    _uid_label->setText(QStringLiteral("ID：%1").arg(uid));
    _avatar_label->setPixmap(makeAvatar(icon, shown_name, 80));
}
