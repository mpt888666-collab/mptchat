//
// Created by mpt on 2026/8/4.
//

#ifndef MPTCHAT_MESSAGETEXTEDIT_H
#define MPTCHAT_MESSAGETEXTEDIT_H
#include <QWidget>
#include <QTextEdit>
enum class ChatMsgType;
class FileCardWidget;
struct MsgInfo;
class MessageTextEdit : public QTextEdit{
    Q_OBJECT
public:
    explicit MessageTextEdit(QWidget *parent = nullptr);

    QVector<std::shared_ptr<MsgInfo>> getMsgList();

    void clearGetMsgList();

    void insertFromMimeData(const QMimeData *source) override;

private:
    void insertMsgList(QVector<std::shared_ptr<MsgInfo>> &list, ChatMsgType msgtype,
        QString text_or_url, QPixmap preview_pix, QString unique_name, uint64_t total_size, QString md5);

    QVector<std::shared_ptr<MsgInfo>> _msgList;
    QVector<std::shared_ptr<MsgInfo>> _getMsgList;

    FileCardWidget * _file_card_widget;
    int _objType;

};


#endif //MPTCHAT_MESSAGETEXTEDIT_H