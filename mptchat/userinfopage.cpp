//
// Created by mpt on 2026/8/24.
//

#include "userinfopage.h"
#include "ui_UserInfoPage.h"

#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QUuid>
#include <QCryptographicHash>

#include "global.h"
#include "HttpMgr.h"
#include "ImageCropperDialog.h"
#include "TCPFileMgr.h"
#include "usermgr.h"

UserInfoPage::UserInfoPage(QWidget *parent) : QWidget(parent), ui(new Ui::UserInfoPage) {
    ui->setupUi(this);

    // 用户名是登录账号，只读展示
    ui->name_label_2->setReadOnly(true);
    ui->name_label_2->setFocusPolicy(Qt::NoFocus);
    ui->name_label_2->setToolTip(QStringLiteral("登录账号不可修改"));
    ui->name_label_2->setStyleSheet(QStringLiteral("color:#8a8a8a;"));

    ui->nick_edit->setMaxLength(64);
    ui->desc_edit->setMaxLength(512);

    // 性别：下拉选择，提交时按索引转成 0=男 / 1=女
    ui->sex_combo->addItem(QStringLiteral("男"), 0);
    ui->sex_combo->addItem(QStringLiteral("女"), 1);

    connect(TCPFileMgr::instance().get(), &TCPFileMgr::sig_user_info_page_show_icon,
            this, &UserInfoPage::RefreshFromUserMgr);
    connect(ui->up_btn, &QPushButton::clicked, this, &UserInfoPage::slot_up_btn);
    connect(ui->submit_btn, &QPushButton::clicked, this, &UserInfoPage::slot_submit);
    connect(HttpMgr::instance().get(), &HttpMgr::sig_login_mod_finish,
            this, &UserInfoPage::slot_update_finish);
}

UserInfoPage::~UserInfoPage() {
    delete ui;
}

void UserInfoPage::RefreshFromUserMgr() {
    auto info = UserMgr::instance()->GetUserInfo();
    if (!info) {
        return;
    }

    ui->nick_edit->setText(info->_nick.isEmpty() ? info->_name : info->_nick);
    ui->name_label_2->setText(info->_name);
    ui->desc_edit->setText(info->_desc);
    ui->sex_combo->setCurrentIndex(info->_sex == 0 ? 0 : 1);

    if (!info->_icon.isEmpty()) {
        QPixmap pix(UserMgr::GetAvatarLocalPath(info->_icon));
        if (!pix.isNull()) {
            ui->head_label->setPixmap(pix.scaled(ui->head_label->size(),
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation));
            ui->head_label->setScaledContents(true);
        }
    } else {
        ui->head_label->clear();
    }
}

QString UserInfoPage::generateUniqueIconName() {
    return QString("head_%1_%2.png")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

void UserInfoPage::slot_submit() {
    auto info = UserMgr::instance()->GetUserInfo();
    if (!info) {
        return;
    }

    const QString nick = ui->nick_edit->text().trimmed();
    const QString desc = ui->desc_edit->text().trimmed();

    if (nick.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("昵称不能为空。"));
        ui->nick_edit->setFocus();
        return;
    }
    if (nick.length() > 64) {
        QMessageBox::warning(this, tr("提示"), tr("昵称不能超过 64 个字符。"));
        ui->nick_edit->setFocus();
        return;
    }
    if (desc.length() > 512) {
        QMessageBox::warning(this, tr("提示"), tr("个性签名不能超过 512 个字符。"));
        ui->desc_edit->setFocus();
        return;
    }

    QJsonObject jsonObj;
    jsonObj["uid"] = UserMgr::instance()->GetUid();
    jsonObj["token"] = UserMgr::instance()->GetToken();
    jsonObj["nick"] = nick;
    jsonObj["desc"] = desc;
    jsonObj["sex"] = ui->sex_combo->currentIndex();

    ui->submit_btn->setEnabled(false);
    ui->submit_btn->setText(tr("保存中…"));

    HttpMgr::instance()->PostHttpReq(QUrl(gate_url_prefix + "/user_update_info"),
                                     jsonObj, ReqId::ID_UPDATE_USER_INFO, Modules::LOGINMOD);
}

void UserInfoPage::slot_update_finish(ReqId id, QString res, ErrorCodes err) {
    if (id != ReqId::ID_UPDATE_USER_INFO) {
        return;
    }

    ui->submit_btn->setEnabled(true);
    ui->submit_btn->setText(tr("保存"));

    if (err != ErrorCodes::SUCCESS) {
        QMessageBox::warning(this, tr("保存失败"), tr("网络异常，请稍后重试。"));
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    if (!doc.isObject()) {
        QMessageBox::warning(this, tr("保存失败"), tr("服务器返回数据异常。"));
        return;
    }

    const QJsonObject obj = doc.object();
    const int code = obj.value("error").toInt(-1);
    if (code != 0) {
        QMessageBox::warning(this, tr("保存失败"),
                             tr("服务器拒绝了本次修改（错误码 %1）。").arg(code));
        return;
    }

    auto info = UserMgr::instance()->GetUserInfo();
    if (info) {
        info->_nick = obj.value("nick").toString(ui->nick_edit->text().trimmed());
        info->_desc = obj.value("desc").toString(ui->desc_edit->text().trimmed());
        info->_sex = obj.value("sex").toInt(ui->sex_combo->currentIndex());
    }

    QMessageBox::information(this, tr("已保存"), tr("个人资料已更新。"));
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
