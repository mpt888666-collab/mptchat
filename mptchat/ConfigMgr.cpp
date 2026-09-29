//
// Created by mpt on 2026/6/30.
//

#include "ConfigMgr.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMetaType>
#include <QSettings>
#include <QStringList>
#include <QVariant>

namespace {

QString BaseDir()
{
    return QCoreApplication::instance() ? QCoreApplication::applicationDirPath()
                                        : QDir::currentPath();
}

QString ResolveConfigPath()
{
    const QString beside_exe = QDir(BaseDir()).filePath(QStringLiteral("config.ini"));
    if (QFileInfo::exists(beside_exe)) {
        return beside_exe;
    }
    const QString in_cwd = QDir(QDir::currentPath()).filePath(QStringLiteral("config.ini"));
    return QFileInfo::exists(in_cwd) ? in_cwd : beside_exe;
}

QString VariantToString(const QVariant &value)
{
    if (value.metaType().id() == QMetaType::QStringList) {
        return value.toStringList().join(QStringLiteral(","));
    }
    return value.toString();
}

}

ConfigMgr &ConfigMgr::Inst()
{
    static ConfigMgr cfg_mgr;
    return cfg_mgr;
}

ConfigMgr::ConfigMgr()
{
    loadConfig();
    InitPath();
}

void ConfigMgr::loadConfig()
{
    _file_path = ResolveConfigPath();

    if (!QFileInfo::exists(_file_path)) {
        qWarning() << "[ConfigMgr] 配置文件不存在:" << _file_path;
        return;
    }

    QSettings settings(_file_path, QSettings::IniFormat);

    for (const QString &section : settings.childGroups()) {
        settings.beginGroup(section);

        SectionInfo info;
        info._section = section;
        for (const QString &key : settings.childKeys()) {
            info._section_datas[key.toStdString()] =
                VariantToString(settings.value(key)).toStdString();
        }

        settings.endGroup();
        _config_map[section.toStdString()] = info;
    }

    qDebug() << "[ConfigMgr] 已加载 section 数:" << _config_map.size() << "来自" << _file_path;
}

SectionInfo ConfigMgr::operator[](const std::string &section) const
{
    const auto it = _config_map.find(section);
    return it == _config_map.end() ? SectionInfo() : it->second;
}

SectionInfo ConfigMgr::operator[](const char *section) const
{
    return (*this)[std::string(section ? section : "")];
}

SectionInfo ConfigMgr::operator[](const QString &section) const
{
    return (*this)[section.toStdString()];
}

QString ConfigMgr::value(const QString &section, const QString &key,
                         const QString &defaultValue) const
{
    const auto it = _config_map.find(section.toStdString());
    if (it == _config_map.end()) {
        return defaultValue;
    }
    const auto kv = it->second._section_datas.find(key.toStdString());
    return kv == it->second._section_datas.end() ? defaultValue
                                                 : QString::fromStdString(kv->second);
}

std::string SectionInfo::GetValue(const std::string &key) const
{
    const auto it = _section_datas.find(key);
    return it == _section_datas.end() ? std::string() : it->second;
}

std::string SectionInfo::operator[](const std::string &key) const
{
    return GetValue(key);
}

QString SectionInfo::value(const QString &key, const QString &defaultValue) const
{
    const auto it = _section_datas.find(key.toStdString());
    return it == _section_datas.end() ? defaultValue : QString::fromStdString(it->second);
}

bool SectionInfo::contains(const QString &key) const
{
    return _section_datas.find(key.toStdString()) != _section_datas.end();
}

QString ConfigMgr::GetFileOutPath() const
{
    return _static_path;
}

void ConfigMgr::InitPath()
{
    const QString bin_dir    = value(QStringLiteral("Output"), QStringLiteral("Path"));
    const QString static_dir = value(QStringLiteral("Static"), QStringLiteral("Path"),
                                     QStringLiteral("static"));

    QDir dir(BaseDir());
    if (!bin_dir.isEmpty()) {
        dir = QDir(dir.filePath(bin_dir));
    }
    _bin_path = dir.absolutePath();
    _static_path = QDir(_bin_path).filePath(static_dir);

    if (!QDir().mkpath(_static_path)) {
        qWarning() << "[ConfigMgr] 创建静态目录失败:" << _static_path;
    }
}
