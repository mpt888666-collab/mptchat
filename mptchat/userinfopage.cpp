//
// Created by mpt on 2026/8/24.
//

// You may need to build the project (run Qt uic code generator) to get "ui_UserInfoPage.h" resolved

#include "userinfopage.h"
#include "ui_UserInfoPage.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QUuid>

#include "global.h"
#include "ImageCropperDialog.h"
#include "TCPFileMgr.h"
#include "usermgr.h"

UserInfoPage::UserInfoPage(QWidget *parent) : QWidget(parent), ui(new Ui::UserInfoPage) {
    ui->setupUi(this);
    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_user_info_page_show_icon, this, [this](){
        QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(storage_dir);
        if (!dir.exists() && !dir.mkpath(storage_dir)) {
            qDebug() << "头像登录加载失败";
        }else {
            QString head_icon = UserMgr::instance()->GetIcon();
            if (!head_icon.isEmpty()) {
                QString avatar_path = QDir(storage_dir).filePath(QString("avatars/") + head_icon);
                QPixmap pix(avatar_path);
                if (!pix.isNull()) {
                    ui->head_label->setPixmap(pix);
                    ui->head_label->setScaledContents(true);
                } else {
                    qWarning() << "头像加载失败:" << avatar_path;
                }
            } else {
                qDebug() << "icon    ''";
            }
        }
    });
    connect(ui->up_btn, &QPushButton::clicked, this, &UserInfoPage::slot_up_btn);
}

UserInfoPage::~UserInfoPage() {
    delete ui;
}

QString UserInfoPage::generateUniqueIconName() {
    return QString("head_%1_%2.png")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

void UserInfoPage::slot_up_btn() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)")
    );

    if (filename.isEmpty()) {
        return;
    }

    QPixmap input_pixmap;
    if (!input_pixmap.load(filename)) {
        QMessageBox::warning(this, tr("错误"), tr("加载图片失败！请确认已部署 WebP 插件。"), QMessageBox::Ok);
        return;
    }

    QPixmap image = ImageCropperDialog::getCroppedImage(filename, 600, 400, CropperShape::CIRCLE);
    if (image.isNull())
        return;

    QPixmap scaled_pixmap = image.scaled(ui->head_label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (scaled_pixmap.isNull()) {
        return;
    }
    ui->head_label->setPixmap(QPixmap());
    ui->head_label->setPixmap(scaled_pixmap);

    QString storage_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(storage_dir);
    if (!dir.exists() && !dir.mkpath(storage_dir)) {
        QMessageBox::warning(this, tr("错误"), tr("无法创建存储目录，请检查权限或磁盘空间。"));
        return;
    }
    if (!dir.exists("avatars") && !dir.mkpath("avatars")) {
        QMessageBox::warning(this, tr("错误"), tr("无法创建存储目录，请检查权限或磁盘空间。"));
        return;
    }

    QString file_name = generateUniqueIconName();
    QString file_path = dir.filePath("avatars" + QString(QDir::separator()) + file_name);

    if (!scaled_pixmap.save(file_path, "PNG")) {
        QMessageBox::warning(this, tr("保存失败"), tr("头像保存失败，请检查权限或磁盘空间。"));
        return;
    }

    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("打开失败"), tr("文件打开失败"));
        return;
    }

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&file)) {
        qWarning() << "Failed to read data from file:" << file_path;
        file.close();
        return;
    }
    QString file_md5 = hash.result().toHex();

    // Reset to the beginning before reading the first chunk.
    file.seek(0);

    QFileInfo file_info(file_path);
    QString name = file_info.fileName();
    int total_size = static_cast<int>(file_info.size());

    int last_seq = (total_size % MAX_FILE_LEN)
                       ? (total_size / MAX_FILE_LEN + 1)
                       : (total_size / MAX_FILE_LEN);

    QByteArray buffer = file.read(MAX_FILE_LEN);
    if (buffer.isEmpty()) {
        file.close();
        return;
    }

    int seq = 1;
    int trans_size = buffer.size();

    QJsonObject json_obj;
    json_obj["md5"] = file_md5;
    json_obj["name"] = name;
    json_obj["seq"] = seq;
    json_obj["trans_size"] = trans_size;
    json_obj["total_size"] = total_size;
    json_obj["token"] = UserMgr::instance()->GetToken();
    json_obj["uid"] = UserMgr::instance()->GetUid();
    json_obj["last"] = (trans_size >= total_size) ? 1 : 0;
    json_obj["data"] = QString::fromLatin1(buffer.toBase64());
    json_obj["last_seq"] = last_seq;

    auto file_info_ptr = std::make_shared<FileInfo>(file_md5, file_path, name);
    UserMgr::instance()->AddNameFile(file_name, file_info_ptr);

    QJsonDocument doc(json_obj);
    TCPFileMgr::instance()->SendData(ID_UPLOAD_HEAD_ICON_REQ, doc.toJson());
    file.close();
    UserMgr::instance()->SetIcon(name);
    emit TCPFileMgr::instance()->sig_chatDialog_head_icon();
}
