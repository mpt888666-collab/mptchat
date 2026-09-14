//
// Created by mpt on 2026/9/8.
//

#include "GroupAvatarLabel.h"

#include <QPainter>
#include <QPainterPath>

namespace {

// WeChat grid: how many avatars sit in each row, top row first.
// The row that is not full goes on top.
QVector<int> rowLayout(int count) {
    switch (count) {
        case 1: return {1};
        case 2: return {2};
        case 3: return {1, 2};
        case 4: return {2, 2};
        case 5: return {2, 3};
        case 6: return {3, 3};
        case 7: return {1, 3, 3};
        case 8: return {2, 3, 3};
        case 9: return {3, 3, 3};
        default: return {3, 3, 3};
    }
}

void drawAvatarCell(QPainter &painter, const QRectF &cell, const QPixmap &pix, qreal radius) {
    if (cell.width() <= 0 || cell.height() <= 0) {
        return;
    }

    painter.save();
    QPainterPath path;
    path.addRoundedRect(cell, radius, radius);
    painter.setClipPath(path);

    if (pix.isNull()) {
        painter.fillRect(cell, QColor("#EDEDED"));
        painter.restore();
        return;
    }

    // Cover the cell: scale up, then center-crop.
    const QSize target(qCeil(cell.width()), qCeil(cell.height()));
    const QPixmap scaled = pix.scaled(target, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QRectF src((scaled.width() - cell.width()) / 2.0,
                     (scaled.height() - cell.height()) / 2.0,
                     cell.width(), cell.height());
    painter.drawPixmap(cell, scaled, src);

    painter.restore();
}

qreal roundedRadiusOf(const QRectF &box) {
    return qMin(box.width(), box.height()) * 0.22;
}

} // namespace

GroupAvatarLabel::GroupAvatarLabel(QWidget *parent)
    : QLabel(parent)
{
}

void GroupAvatarLabel::setAvatarList(const QVector<QPixmap> &list)
{
    m_avatars = list;
    update();
}

void GroupAvatarLabel::setSpacing(int px)
{
    m_spacing = px;
    update();
}

void GroupAvatarLabel::setMaxShowCount(int cnt)
{
    m_maxShowCount = cnt;
    update();
}

void GroupAvatarLabel::paintEvent(QPaintEvent *event)
{
    // No member list -> this is a private chat avatar (or a single avatar set
    // through setPixmap by SetLabelAvatar). Draw that pixmap as a rounded
    // square so it looks like a WeChat avatar.
    if (m_avatars.isEmpty()) {
        const QPixmap own = pixmap();
        if (own.isNull()) {
            QLabel::paintEvent(event);
            return;
        }

        QPainter single(this);
        single.setRenderHint(QPainter::Antialiasing, true);
        single.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const QRectF box(rect());
        const qreal radius = roundedRadiusOf(box);
        QPainterPath outer;
        outer.addRoundedRect(box, radius, radius);
        single.setClipPath(outer);
        single.fillRect(box, QColor("#E6E6E6"));
        drawAvatarCell(single, box, own, radius);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF box(rect());
    const qreal outerRadius = roundedRadiusOf(box);

    // Rounded square frame + background; the background shows through the gaps.
    QPainterPath outer;
    outer.addRoundedRect(box, outerRadius, outerRadius);
    painter.setClipPath(outer);
    painter.fillRect(box, QColor("#E6E6E6"));

    const int total = qMin(static_cast<int>(m_avatars.size()), qMax(1, m_maxShowCount));
    if (total <= 0) {
        return;
    }

    const QVector<int> rows = rowLayout(total);
    const int rowCount = static_cast<int>(rows.size());
    const qreal gap = qMax(1, m_spacing);
    const qreal rowHeight = (box.height() - gap * (rowCount - 1)) / rowCount;
    const qreal cellRadius = qBound(1.0, box.width() * 0.06, 3.0);

    int index = 0;
    qreal y = box.top();
    for (int i = 0; i < rowCount; ++i) {
        const int inRow = rows[i];
        const qreal cellWidth = (box.width() - gap * (inRow - 1)) / inRow;
        qreal x = box.left();
        for (int j = 0; j < inRow && index < total; ++j, ++index) {
            drawAvatarCell(painter, QRectF(x, y, cellWidth, rowHeight), m_avatars[index], cellRadius);
            x += cellWidth + gap;
        }
        y += rowHeight + gap;
    }
}