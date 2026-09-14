#ifndef MPTCHAT_FILEBUBBLE_H
#define MPTCHAT_FILEBUBBLE_H

#include "chatitembase.h"
#include <QLabel>
#include <QtGlobal>

class QProgressBar;

class FileBubble : public ChatItemBase
{
    Q_OBJECT
public:
    explicit FileBubble(ChatRole role, const QString& fileName, const QString& filePath,
                        int ownerUid = 0, bool localReady = false, QWidget *parent = nullptr);

private:
    QString m_fileName;
    QString m_filePath;
    int m_ownerUid = 0;
    bool m_localReady = false;
    bool m_downloading = false;
    bool m_openOnFinish = false;

    QLabel* m_iconLabel = nullptr;
    QLabel* m_nameLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;

    int _cur_size = 0;
    int _total_size = 0;

private slots:
    void startDownload(bool openWhenDone);
    void onDownloadProgress(const QString &name, qint64 current, qint64 total);
    void onDownloadFinished(const QString &name, const QString &localPath);
    void onDownloadFailed(const QString &name, int error);

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif //MPTCHAT_FILEBUBBLE_H
