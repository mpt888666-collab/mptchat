#include <QApplication>
#include <QPushButton>
#include "mainwindow.h"
#include <QFile>
#include <iostream>
#include "global.h"
#include <QSettings>
#include <QDir>
#include <QDebug>
#include "resetdialog.h"
#include "chatdialog.h"
#include "TCPFileMgr.h"
#include "SherpaOnnxRecognizer.h"
#include <cstring>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    //获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    // 拼接文件名
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path +
                            QDir::separator() + fileName);
    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://"+gate_host+":"+gate_port;
    QFile file(":/style/style.qss");
    if (file.open(QIODevice::ReadOnly))
    {
        QString style = file.readAll();
        a.setStyleSheet(style);
        file.close();
        std::cout << "qss in sucsece" << std::endl;
    }
    RecognizerThread recognizer_thread;
    FileTcpThread file_tcp_thread;
    TCPThread tcp_thread;

    mainwindow w;
    w.show();

    return QApplication::exec();
}
