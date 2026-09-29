//
// Created by mpt on 2026/6/30.
//
#ifndef MPTCHAT_CONFIGMGR_H
#define MPTCHAT_CONFIGMGR_H

#include <QString>
#include <map>
#include <string>
class SectionInfo {
public:
    SectionInfo() = default;
    ~SectionInfo() = default;

    std::string GetValue(const std::string &key) const;
    std::string operator[](const std::string &key) const;

    QString value(const QString &key, const QString &defaultValue = QString()) const;
    bool    contains(const QString &key) const;
    QString section() const { return _section; }

    std::map<std::string, std::string> _section_datas;
    QString _section;
};

class ConfigMgr {
public:
    static ConfigMgr &Inst();

    ConfigMgr(const ConfigMgr &) = delete;
    ConfigMgr &operator=(const ConfigMgr &) = delete;

    SectionInfo operator[](const char *section) const;
    SectionInfo operator[](const std::string &section) const;
    SectionInfo operator[](const QString &section) const;

    QString value(const QString &section, const QString &key,
                  const QString &defaultValue = QString()) const;

    QString GetFileOutPath() const;

    QString filePath() const { return _file_path; }

private:
    ConfigMgr();
    ~ConfigMgr() = default;

    void loadConfig();
    void InitPath();

    std::map<std::string, SectionInfo> _config_map;
    QString _file_path;
    QString _bin_path;
    QString _static_path;
};

#endif //MPTCHAT_CONFIGMGR_H
