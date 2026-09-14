#include "FileBubble.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QMouseEvent>
#include <QFileInfo>
#include <QDebug>

#include "TCPFileMgr.h"
#include "UserMgr.h"

FileBubble::FileBubble(ChatRole role, const QString &fileName, const QString &filePath,
                       int ownerUid, bool localReady, QWidget *parent)
    : ChatItemBase(role, parent)
    , m_fileName(fileName)
    , m_filePath(filePath)
    , m_ownerUid(ownerUid)
    , m_localReady(localReady)
{
    QWidget* bubbleInner = new QWidget();
    QHBoxLayout* innerLayout = new QHBoxLayout(bubbleInner);
    innerLayout->setContentsMargins(12, 10, 12, 10);
    innerLayout->setSpacing(10);

    m_iconLabel = new QLabel();
    m_iconLabel->setFixedSize(32,32);
    m_iconLabel->setPixmap(QIcon::fromTheme("document").pixmap(32,32));

    m_nameLabel = new QLabel(m_fileName);
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setMaximumWidth(260);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setMinimumWidth(200);
    m_progressBar->hide();

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(6);
    textLayout->addWidget(m_nameLabel);
    textLayout->addWidget(m_progressBar);

    innerLayout->addWidget(m_iconLabel);
    innerLayout->addLayout(textLayout);

    if(role == ChatRole::Self)
    {
        bubbleInner->setStyleSheet(R"(
            QWidget{
                background-color:#95ec69;
                border-radius:12px;
            }
            QLabel{
                background-color:transparent;
            }
        )");
    }
    else
    {
        bubbleInner->setStyleSheet(R"(
            QWidget{
                background-color:#ffffff;
                border-radius:12px;
                border:1px solid #e5e5e5;
            }
            QLabel{
                background-color:transparent;
            }
        )");
    }

    this->setWidget(bubbleInner);

    auto fileMgr = TCPFileMgr::instance().get();
    connect(fileMgr, &TCPFileMgr::sig_file_download_progress,
            this, &FileBubble::onDownloadProgress);
    connect(fileMgr, &TCPFileMgr::sig_file_download_finished,
            this, &FileBubble::onDownloadFinished);
    connect(fileMgr, &TCPFileMgr::sig_file_download_failed,
            this, &FileBubble::onDownloadFailed);
}

void FileBubble::mouseReleaseEvent(QMouseEvent *event)
{
    ChatItemBase::mouseReleaseEvent(event);
    if (m_downloading) {
        return;
    }

    if (m_localReady && QFileInfo::exists(m_filePath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_filePath));
        return;
    }

    startDownload(true);
}

void FileBubble::startDownload(bool openWhenDone)
{
    if (m_downloading) {
        return;
    }

    m_downloading = true;
    m_openOnFinish = openWhenDone;
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_nameLabel->setText(m_fileName);

    if (!UserMgr::instance()->IsDownLoading(m_fileName)) {
        auto download = std::make_shared<DownloadInfo>(m_fileName, m_filePath, m_ownerUid);
        UserMgr::instance()->AddDownloadFile(m_fileName, download);
        TCPFileMgr::instance()->SendDownloadInfo(download);
    }
}

void FileBubble::onDownloadProgress(const QString &name, qint64 current, qint64 total)
{
    if (name != m_fileName) {
        return;
    }

    int percent = 0;
    if (total > 0) {
        percent = static_cast<int>((current * 100) / total);
    }
    m_progressBar->setValue(percent);
    m_progressBar->show();
}

void FileBubble::onDownloadFinished(const QString &name, const QString &localPath)
{
    if (name != m_fileName) {
        return;
    }

    m_downloading = false;
    m_progressBar->hide();
    m_nameLabel->setText(m_fileName);
    if (m_openOnFinish && QFileInfo::exists(localPath)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
    }
}

void FileBubble::onDownloadFailed(const QString &name, int error)
{
    if (name != m_fileName) {
        return;
    }

    m_downloading = false;
    m_progressBar->hide();
    qWarning() << "download file failed:" << m_fileName << "error:" << error;
    m_nameLabel->setText(QString("%1 (下载失败)").arg(m_fileName));
}
