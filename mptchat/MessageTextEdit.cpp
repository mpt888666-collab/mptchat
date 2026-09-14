//
// Created by mpt on 2026/8/4.
//

#include "MessageTextEdit.h"
#include "MessageTextEdit.h"

#include <QDir>

#include "global.h"
#include <QTextBlock>
#include <QBuffer>
#include <QUuid>
#include <QCryptographicHash>
#include <QUrl>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QTextCursor>
#include <QMimeData>
#include "FileCardWidget.h"
MessageTextEdit::MessageTextEdit(QWidget * parent) : QTextEdit(parent) {
    setAcceptDrops(true);
    auto doc = this->document();
    _file_card_widget = new FileCardWidget(doc);
    _objType = FileCardWidget::GetObjType();

    doc->documentLayout()->registerHandler(_objType, _file_card_widget);
}

QVector<std::shared_ptr<MsgInfo>> MessageTextEdit::getMsgList() {
    _getMsgList.clear();

    auto doc = this->document();
    QString textBuf;
    for (QTextBlock block = doc->begin(); block != doc->end(); block = block.next()) {
        QTextBlock::iterator it;
        for (it = block.begin(); it != block.end(); it++) {
            QTextFragment fragment = it.fragment();
            if (!fragment.isValid()) {
                continue;
            }

            QTextCharFormat fmt = fragment.charFormat();

            if (fmt.isImageFormat()) {
                if (!textBuf.isEmpty()) {
                    insertMsgList(_getMsgList, ChatMsgType::TEXT, textBuf, QPixmap(), QString(), 0, QString());
                    textBuf.clear();
                }

                QTextImageFormat imageFmt = fmt.toImageFormat();
                QString name = imageFmt.name();
                QUrl resUrl(name);
                QVariant var = doc->resource(QTextDocument::ImageResource, resUrl);
                if (var.isValid() && var.canConvert<QImage>()) {
                    QImage image = var.value<QImage>();
                    QByteArray ba;
                    QBuffer buffer(&ba);
                    buffer.open(QIODevice::WriteOnly);
                    image.save(&buffer, "PNG");
                    qint64 total_size = ba.size();
                    QString md5 = QString::fromLatin1(QCryptographicHash::hash(ba, QCryptographicHash::Md5).toHex());
                    QString unique_name = QUuid::createUuid().toString(QUuid::WithoutBraces) + ".png";

                    QString local_path = QDir::temp().filePath(unique_name);
                    QFile image_file(local_path);
                    if (image_file.open(QIODevice::WriteOnly)) {
                        image_file.write(ba);
                        image_file.close();
                    }

                    insertMsgList(_getMsgList, ChatMsgType::PIC, local_path,
                        QPixmap::fromImage(image), unique_name, static_cast<uint64_t>(total_size), md5);
                }
            } else if (fragment.text() == QString(QChar::ObjectReplacementCharacter)
                       && fmt.objectType() == _objType
                       && fmt.property(QTextFormat::UserProperty).isValid()
                       && fmt.property(QTextFormat::UserProperty).toString() == "file") {
                QString fileName = fmt.property(QTextFormat::UserProperty + 1).toString();
                QString filePath = fmt.property(QTextFormat::UserProperty + 2).toString();
                qint64 fileSize = fmt.property(QTextFormat::UserProperty + 3).toLongLong();
                if (!textBuf.isEmpty())
                {
                    insertMsgList(_getMsgList, ChatMsgType::TEXT, textBuf, QPixmap(), QString(), 0, QString());
                    textBuf.clear();
                }

                QFile file(filePath);
                if (file.open(QFile::ReadOnly)) {
                    QString unique_name = QUuid::createUuid().toString() + "_" + fileName;
                    QString md5 = QString::fromLatin1(QCryptographicHash::hash(file.readAll(), QCryptographicHash::Md5).toHex());
                    insertMsgList(_getMsgList, ChatMsgType::FILE, filePath,
                        QPixmap(), unique_name, static_cast<uint64_t>(fileSize), md5);
                    file.close();


                }

                qDebug() << "this is send a file ............................................................";

            } else {
                QString text = fragment.text();
                text.remove(QChar::ObjectReplacementCharacter);
                textBuf += text;
            }
        }
    }

    if (!textBuf.isEmpty()) {
        insertMsgList(_getMsgList, ChatMsgType::TEXT, textBuf, QPixmap(), QString(), 0, QString());
        textBuf.clear();
    }

    _msgList.clear();
    this->clear();
    return _getMsgList;
}
void MessageTextEdit::insertMsgList(QVector<std::shared_ptr<MsgInfo>> &list, ChatMsgType msgtype,
    QString text_or_url, QPixmap preview_pix,
    QString unique_name, uint64_t total_size, QString md5) {

    auto msg_info = std::make_shared<MsgInfo>(msgtype, text_or_url, preview_pix, unique_name, total_size, md5);
    list.append(msg_info);

}

void MessageTextEdit::clearGetMsgList() {
    _getMsgList.clear();
}

void MessageTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (!source) {
        return;
    }

    const int IMG_MAX_WIDTH = 100;
    const int IMG_MAX_HEIGHT = 100;
    bool inserted = false;

    // 1. clipboard screenshot 剪贴板图片
    if (source->hasImage()) {
        QImage img = source->imageData().value<QImage>();
        if (!img.isNull()) {
            QUrl virtualKey(QString("clipboard_img_%1").arg(QDateTime::currentMSecsSinceEpoch()));
            document()->addResource(QTextDocument::ImageResource, virtualKey, img);

            QTextImageFormat fmt;
            QSize scaled = img.size();
            if (scaled.width() > IMG_MAX_WIDTH || scaled.height() > IMG_MAX_HEIGHT) {
                scaled.scale(IMG_MAX_WIDTH, IMG_MAX_HEIGHT, Qt::KeepAspectRatio);
            }
            fmt.setWidth(scaled.width());
            fmt.setHeight(scaled.height());
            fmt.setName(virtualKey.toString());
            fmt.setProperty(QTextFormat::UserProperty, "image");
            fmt.setProperty(QTextFormat::UserProperty + 1, "");
            fmt.setProperty(QTextFormat::UserProperty + 2,
                QString("screenshot_%1.png").arg(QDateTime::currentMSecsSinceEpoch()));

            QTextCursor cursor = textCursor();
            cursor.insertImage(fmt);
            setTextCursor(cursor);
            inserted = true;
        }
    }

    // 2. drop files: images insert image, other files insert file‑info text
    if (source->hasUrls()) {
        const QStringList imgSuffix{ "png", "jpg", "jpeg", "bmp", "gif", "webp" };
        QTextCursor cursor = textCursor();

        for (const QUrl &url : source->urls()) {
            if (!url.isLocalFile()) {
                continue;
            }
            QString localPath = url.toLocalFile();
            QFileInfo fi(localPath);
            QString suffix = fi.suffix().toLower();

            if (imgSuffix.contains(suffix)) {
                // 本地图片：插入图片
                QImage img(localPath);
                if (img.isNull()) {
                    continue;
                }
                QTextImageFormat fmt;
                QSize scaled = img.size();
                if (scaled.width() > IMG_MAX_WIDTH || scaled.height() > IMG_MAX_HEIGHT) {
                    scaled.scale(IMG_MAX_WIDTH, IMG_MAX_HEIGHT, Qt::KeepAspectRatio);
                }
                fmt.setWidth(scaled.width());
                fmt.setHeight(scaled.height());

                document()->addResource(QTextDocument::ImageResource, url, img);
                fmt.setName(url.toString());
                fmt.setProperty(QTextFormat::UserProperty, "image");
                fmt.setProperty(QTextFormat::UserProperty + 1, localPath);
                fmt.setProperty(QTextFormat::UserProperty + 2, fi.fileName());

                cursor.insertImage(fmt);
                inserted = true;
            } else {
                // ========== 普通文件：插入自定义文件卡片对象 ==========
                QTextCharFormat fileFmt;
                fileFmt.setObjectType(_objType); //拷贝模板format
                fileFmt.setProperty(QTextFormat::UserProperty, "file");
                fileFmt.setProperty(QTextFormat::UserProperty + 1, fi.fileName());
                fileFmt.setProperty(QTextFormat::UserProperty + 2, localPath);
                fileFmt.setProperty(QTextFormat::UserProperty + 3, fi.size());

                // 插入对象替换字符
                cursor.insertText(QString(QChar::ObjectReplacementCharacter), fileFmt);
                cursor.setCharFormat(QTextCharFormat());
                inserted = true;
            }
        }
        setTextCursor(cursor);
    }

    if (inserted) {
        return;
    }

    // 3. fallback text/html/plain text
    QTextEdit::insertFromMimeData(source);
}
