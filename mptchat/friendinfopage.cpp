//
// Created by mpt on 2026/8/15.
//

#include "friendinfopage.h"

#include <utility>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QSize>

#include "ui_FriendInfoPage.h"
#include "userdata.h"
#include "UserMgr.h"
#include "TCPFileMgr.h"
#include "global.h"

namespace {
constexpr int kAvatarRadius = 12;
constexpr int kAvatarFallbackSize = 64;

// 居中裁切成正方形后再切圆角，微信好友资料页的头像就是这个效果
QPixmap roundedPixmap(const QPixmap &src, const QSize &size, int radius) {
    if (src.isNull() || size.isEmpty()) {
        return QPixmap();
    }

    QPixmap scaled = src.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = qMax(0, (scaled.width() - size.width()) / 2);
    const int y = qMax(0, (scaled.height() - size.height()) / 2);
    scaled = scaled.copy(x, y, size.width(), size.height());

    QPixmap result(size);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath clip;
    clip.addRoundedRect(QRectF(QPointF(0, 0), QSizeF(size)), radius, radius);
    painter.setClipPath(clip);
    painter.drawPixmap(0, 0, scaled);
    painter.end();

    return result;
}

// 头像还没下载好时先显示灰底 + 首字，避免出现一块空白
QPixmap placeholderPixmap(const QSize &size, const QString &name, int radius) {
    QPixmap result(size);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0xd7, 0xdb, 0xe0));
    painter.drawRoundedRect(QRectF(QPointF(0, 0), QSizeF(size)), radius, radius);

    const QString trimmed = name.trimmed();
    QFont font = painter.font();
    font.setPixelSize(qMax(16, size.height() / 2 - 2));
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QColor(0xff, 0xff, 0xff));
    painter.drawText(QRect(QPoint(0, 0), size), Qt::AlignCenter,
                     trimmed.isEmpty() ? QStringLiteral("?") : trimmed.left(1));
    painter.end();

    return result;
}
} // namespace

FriendInfoPage::FriendInfoPage(QWidget *parent) : QWidget(parent), ui(new Ui::FriendInfoPage) {
    ui->setupUi(this);
    connect(ui->send_msg_label, &ClickedLabel::clicked, this, &FriendInfoPage::on_msg_chat_clicked);

    // 陌生人第一次查看资料时头像要现下，下载完刷新一下本页
    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_avatar_downloaded, this,
            [this](const QString &name) {
                if (_user_info && _user_info->_icon == name) {
                    refreshAvatar();
                }
            });

    ui->send_msg_label->setCursor(Qt::PointingHandCursor);
    ui->voice_label->setCursor(Qt::PointingHandCursor);
    ui->video_label->setCursor(Qt::PointingHandCursor);
}

FriendInfoPage::~FriendInfoPage() {
    delete ui;
}

void FriendInfoPage::setInfo(std::shared_ptr<UserInfo> user_info) {
    _user_info = user_info;
    if (!_user_info) {
        return;
    }

    ui->name_label->setText(_user_info->_name);
    ui->sex_label->setText(_user_info->_sex == 0 ? QStringLiteral("男") : QStringLiteral("女"));
    ui->sex_label->setProperty("sex", _user_info->_sex == 0 ? "male" : "female");
    repolish(ui->sex_label);
    ui->desc_label->setText(_user_info->_desc.isEmpty() ? QStringLiteral("暂无个性签名")
                                                        : _user_info->_desc);

    refreshAvatar();
}

void FriendInfoPage::refreshAvatar() {
    if (!_user_info) {
        return;
    }

    QSize avatar_size = ui->head_label->size();
    if (avatar_size.isEmpty()) {
        avatar_size = QSize(kAvatarFallbackSize, kAvatarFallbackSize);
    }

    const QString icon = _user_info->_icon;
    QPixmap pix;
    const QString path = UserMgr::GetAvatarLocalPath(icon);
    if (!path.isEmpty()) {
        pix.load(path);
    }

    if (pix.isNull()) {
        // 本地没有就去资源服务器拉，下载完成后再刷新
        UserMgr::instance()->EnsureAvatarDownloaded(icon, _user_info->_uid);
        ui->head_label->setPixmap(placeholderPixmap(avatar_size, _user_info->_name, kAvatarRadius));
    } else {
        ui->head_label->setPixmap(roundedPixmap(pix, avatar_size, kAvatarRadius));
    }
}

void FriendInfoPage::on_msg_chat_clicked()
{
    qDebug() << "msg chat btn clicked";
    emit sig_jump_chat_item(_user_info);
}
