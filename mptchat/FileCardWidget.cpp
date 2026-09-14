#include "FileCardWidget.h"
#include <QPainter>
#include <QFontMetricsF>

FileCardWidget::FileCardWidget(QObject *parent)
    : QObject(parent)
{
}

int FileCardWidget::GetObjType()
{
    return FILE_CARD_OBJECT_TYPE;
}

QString FileCardWidget::formatFileSize(qint64 bytes)
{
    if(bytes < 1024)
        return QString("%1 B").arg(bytes);
    else if(bytes < 1024 * 1024)
        return QString("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
    else if(bytes < 1024LL * 1024 * 1024)
        return QString("%1 MB").arg(QString::number(bytes / (1024.0*1024), 'f',1));
    else
        return QString("%1 GB").arg(QString::number(bytes / (1024.0*1024*1024), 'f',1));
}

QSizeF FileCardWidget::intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(doc);
    Q_UNUSED(posInDocument);
    return QSizeF(220, 64);
}

void FileCardWidget::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(doc);
    Q_UNUSED(posInDocument);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QString fileName = format.property(QTextFormat::UserProperty + 1).toString();
    QString filePath = format.property(QTextFormat::UserProperty + 2).toString();
    qint64 fileBytes = format.property(QTextFormat::UserProperty + 3).toLongLong();
    QString sizeText = formatFileSize(fileBytes);

    QRectF cardRect = rect.adjusted(2,2,-2,-2);

    painter->setPen(QColor(0xDCDCDC));
    painter->setBrush(QColor(0xF7F7F7));
    painter->drawRoundedRect(cardRect, 8,8);

    QRectF iconRect(cardRect.left() + 10, cardRect.top() + 16,32,32);
    painter->setBrush(QColor(0x4488DD));
    painter->setPen(Qt::white);
    painter->drawRoundedRect(iconRect,4,4);
    painter->drawText(iconRect, Qt::AlignCenter, "F");

    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::black);
    QRectF nameRect(iconRect.right() + 10, cardRect.top()+12, cardRect.width() - iconRect.width() - 24,22);
    painter->drawText(nameRect, Qt::TextSingleLine | Qt::AlignVCenter, fileName);

    font.setBold(false);
    painter->setFont(font);
    painter->setPen(QColor(0x707070));
    QRectF sizeRect(nameRect.left(), nameRect.bottom() + 4, nameRect.width(),18);
    painter->drawText(sizeRect, Qt::TextSingleLine | Qt::AlignVCenter, sizeText);

    painter->restore();
}
