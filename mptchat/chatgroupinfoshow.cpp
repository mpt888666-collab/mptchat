//
// Created by mpt on 2026/9/21.
//

#include "chatgroupinfoshow.h"

#include <QColor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
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

ChatGroupInfoShow::ChatGroupInfoShow(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("群聊信息"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(320, 500);
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
        QScrollArea { border: none; background: transparent; }
        QWidget#memberRow { background: transparent; }
        QLabel#memberName { color: #222222; font-size: 13px; }
        QLabel#sectionTitle { color: #9a9a9a; font-size: 12px; }
    )");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *header = new QWidget(this);
    auto *header_layout = new QVBoxLayout(header);
    header_layout->setContentsMargins(0, 24, 0, 16);
    header_layout->setSpacing(6);

    _avatar_label = new QLabel(header);
    _avatar_label->setFixedSize(72, 72);
    _avatar_label->setAlignment(Qt::AlignCenter);

    _name_label = new QLabel(header);
    _name_label->setObjectName("infoName");
    _name_label->setAlignment(Qt::AlignCenter);

    _count_label = new QLabel(header);
    _count_label->setObjectName("infoUid");
    _count_label->setAlignment(Qt::AlignCenter);

    header_layout->addWidget(_avatar_label, 0, Qt::AlignHCenter);
    header_layout->addWidget(_name_label);
    header_layout->addWidget(_count_label);
    root->addWidget(header);

    root->addWidget(makeDivider(this));

    auto *members_title = new QLabel(QStringLiteral("群成员"), this);
    members_title->setObjectName("sectionTitle");
    members_title->setContentsMargins(18, 12, 0, 6);
    root->addWidget(members_title);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *member_container = new QWidget(scroll);
    member_container->setObjectName("memberRow");
    _member_layout = new QVBoxLayout(member_container);
    _member_layout->setContentsMargins(0, 0, 0, 6);
    _member_layout->setSpacing(0);
    _member_layout->addStretch();
    scroll->setWidget(member_container);
    root->addWidget(scroll, 1);

    root->addWidget(makeDivider(this));

    auto *invite_btn = new QPushButton(QStringLiteral("邀请好友入群"), this);
    invite_btn->setObjectName("infoAction");
    invite_btn->setFixedHeight(52);
    invite_btn->setCursor(Qt::PointingHandCursor);
    connect(invite_btn, &QPushButton::clicked, this, &ChatGroupInfoShow::onInviteClicked);
    root->addWidget(invite_btn);

    auto *clear_btn = new QPushButton(QStringLiteral("清空聊天记录"), this);
    clear_btn->setObjectName("infoAction");
    clear_btn->setFixedHeight(52);
    clear_btn->setCursor(Qt::PointingHandCursor);
    connect(clear_btn, &QPushButton::clicked, this, &ChatGroupInfoShow::sig_clear_chat_requested);
    root->addWidget(clear_btn);

    auto *bottom_space = new QWidget(this);
    bottom_space->setFixedHeight(10);
    root->addWidget(bottom_space);
}

ChatGroupInfoShow::~ChatGroupInfoShow() = default;

void ChatGroupInfoShow::SetGroupInfo(const QString &name, const QVector<int> &members, int owner_uid)
{
    _members = members;
    _owner_uid = owner_uid;
    const QString shown_name = name.isEmpty() ? QStringLiteral("群聊") : name;

    _name_label->setText(shown_name);
    _count_label->setText(QStringLiteral("群成员 %1 人").arg(members.size()));
    _avatar_label->setPixmap(makeAvatar(QString(), shown_name, 72));

    // 重建成员列表
    while (_member_layout->count() > 1) {
        QLayoutItem *item = _member_layout->takeAt(0);
        if (item) {
            if (QWidget *w = item->widget()) {
                w->deleteLater();
            }
            delete item;
        }
    }

    for (int uid : members) {
        auto info = UserMgr::instance()->GetUserInfoById(uid);
        if (uid == UserMgr::instance()->GetUid()) {
            info = UserMgr::instance()->GetUserInfo();
        }
        const QString member_name = info && !info->_name.isEmpty()
            ? info->_name
            : QStringLiteral("用户 %1").arg(uid);
        const QString icon = info ? info->_icon : QString();

        auto *row = new QWidget();
        row->setObjectName("memberRow");
        row->setFixedHeight(50);
        auto *row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(18, 0, 18, 0);
        row_layout->setSpacing(10);

        auto *avatar = new QLabel(row);
        avatar->setFixedSize(32, 32);
        avatar->setPixmap(makeAvatar(icon, member_name, 32));

        auto *name_label = new QLabel(member_name, row);
        name_label->setObjectName("memberName");

        row_layout->addWidget(avatar);
        row_layout->addWidget(name_label, 1);

        if (uid == _owner_uid) {
            auto *owner_tag = new QLabel(QStringLiteral("群主"), row);
            owner_tag->setStyleSheet(QStringLiteral("color:#f0a020;font-size:12px;"));
            row_layout->addWidget(owner_tag);
        }

        if (uid == UserMgr::instance()->GetUid()) {
            auto *self_tag = new QLabel(QStringLiteral("我"), row);
            self_tag->setStyleSheet(QStringLiteral("color:#07c160;font-size:12px;"));
            row_layout->addWidget(self_tag);
        }

        if (UserMgr::instance()->GetUid() == _owner_uid &&
            uid != UserMgr::instance()->GetUid()) {
            auto *kick_btn = new QPushButton(QStringLiteral("移出"), row);
            kick_btn->setFixedSize(48, 26);
            kick_btn->setCursor(Qt::PointingHandCursor);
            kick_btn->setStyleSheet(QStringLiteral(
                "QPushButton{border:1px solid #e5b4b4;border-radius:5px;"
                "background:#fff7f7;color:#d94b4b;font-size:12px;}"
                "QPushButton:hover{background:#ffecec;}"));
            connect(kick_btn, &QPushButton::clicked, this, [this, uid]() {
                emit sig_kick_member(uid);
            });
            row_layout->addWidget(kick_btn);
        }

        _member_layout->insertWidget(_member_layout->count() - 1, row);
    }
}

void ChatGroupInfoShow::onInviteClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("邀请好友入群"));
    dialog.resize(280, 360);

    auto *layout = new QVBoxLayout(&dialog);
    auto *list = new QListWidget(&dialog);
    const auto friends = UserMgr::instance()->GetFriendList();
    for (const auto &friend_info : friends) {
        if (!friend_info || _members.contains(friend_info->_uid)) {
            continue;
        }
        auto *item = new QListWidgetItem(
            friend_info->_name.isEmpty() ? friend_info->_nick : friend_info->_name, list);
        item->setData(Qt::UserRole, friend_info->_uid);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    layout->addWidget(list);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QVector<int> selected;
    for (int row = 0; row < list->count(); ++row) {
        auto *item = list->item(row);
        if (item->checkState() == Qt::Checked) {
            selected.push_back(item->data(Qt::UserRole).toInt());
        }
    }
    if (!selected.isEmpty()) {
        emit sig_invite_members(selected);
    }
}
