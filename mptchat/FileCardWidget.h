//
// Created by mpt on 2026/9/4.
//

#ifndef MPTCHAT_FILECARDWIDGET_H
#define MPTCHAT_FILECARDWIDGET_H
#include <QObject>
#include <QTextObjectInterface>
#include "global.h"
class FileCardWidget : public QObject, public QTextObjectInterface {
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)
public:
    explicit FileCardWidget(QObject * parient = nullptr);
    QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;


    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *doc, int posInDocument, const QTextFormat &format) override;

    static int GetObjType();
private:
    static constexpr int FILE_CARD_OBJECT_TYPE = QTextFormat::UserObject + 1;

    QString formatFileSize(qint64 bytes);
};


#endif //MPTCHAT_FILECARDWIDGET_H