//
// Created by mpt on 2026/7/9.
//

#ifndef MPTCHAT_USERMGR_H
#define MPTCHAT_USERMGR_H

#include "global.h"
#include "singleton.h"
#include "userdata.h"
#include <QSet>
struct ApplyInfo;
class QLabel;
struct UserInfo;
struct AuthInfo;
class ChatThreadData;
class UserMgr:public QObject,public Singleton<UserMgr>,
        public std::enable_shared_from_this<UserMgr>
{
    Q_OBJECT
public:
    friend class Singleton<UserMgr>;
    ~ UserMgr() override;
    void SetUserInfo(std::shared_ptr<UserInfo> user_info);
    void SetToken(QString token);

    QString GetName();
    QString GetNick();
    QString GetIcon();
    QString GetDesc();
    void SetIcon(QString name);

    static QString GetAvatarLocalPath(const QString &iconFileName);
    static void SetLabelAvatar(QLabel *label, const QString &iconFileName);
    bool EnsureAvatarDownloaded(const QString &iconFileName, int owner_uid);

    bool IsDownLoading(QString name);

    void AddLabelToReset(QString path, QLabel *label);

    void AddDownloadFile(QString name, std::shared_ptr<DownloadInfo> file_info);

    void ResetLabelIcon(QString path);

    int GetUid();
    std::shared_ptr<UserInfo> GetUserInfo();
    void AddFileInfoByMD5(QString md5, std::shared_ptr<FileInfo> info);
    std::shared_ptr<FileInfo> GetFileInfoByMD5(QString md5);
    void AddNameFile(QString name, std::shared_ptr<FileInfo> info);
    std::shared_ptr<FileInfo> GetNameFile(QString name);

    void AddApplyList(std::shared_ptr<ApplyInfo> app);
    bool AlreadyApply(int uid);
    void AppendApplyList(QJsonArray array);
    void AppendFriendList(QJsonArray array);
    bool CheckFriendById(int uid);
    void AddChatThreadData(std::shared_ptr<ChatThreadData>);
    void SetLastChatThreadId(int);
    int GetChatThreadByUid(int uid);

    std::shared_ptr<ChatThreadData> GetChatThreadDataByThreadId(int thread_id);

    std::shared_ptr<ChatThreadData> GetCurLoadData();

    std::shared_ptr<ChatThreadData> GetNextLoadData();

    void AddFriend(std::shared_ptr<AuthInfo> user);
    void AddFriend(std::shared_ptr<AuthRsp> user);

    std::vector<std::shared_ptr<ApplyInfo>> GetApplyList();
    std::vector<std::shared_ptr<UserInfo>> GetFriendList();
    std::shared_ptr<UserInfo> GetFriendById(int uid);

    std::shared_ptr<UserInfo> GetUserInfoById(int uid);
    void AddGroupMemberInfo(std::shared_ptr<UserInfo> info);

    void AppendFriendChatMsg(int friend_uid, QJsonArray contents);
    QVector<QJsonObject> GetFriendChatMsgs(int friend_uid);
    QString GetToken() {
        return _token;
    }

    void RmvDownloadFile(QString name);
    std::shared_ptr<DownloadInfo> GetDownloadInfo(QString name);

    std::shared_ptr<MsgInfo> GetTransFileByName(QString name);

    void AddTransFile(QString name, std::shared_ptr<MsgInfo> msg_info);

    void Clear();


private:
    UserMgr();
    std::mutex _down_load_mtx;
    std::mutex _file_msg_mtx;
    std::shared_ptr<UserInfo> _user_info;
    QString _token;
    int _last_thread_id;
    int _cur_load_chat_index;
    std::vector<std::shared_ptr<ApplyInfo>> _apply_list;
    std::vector<std::shared_ptr<UserInfo>> _friend_list;
    QMap<int, std::shared_ptr<UserInfo>> _friend_map;
    QMap<int, std::shared_ptr<UserInfo>> _group_member_infos;
    QMap<int, QVector<QJsonObject>> _friend_chat_msgs;
    QMap<int, std::shared_ptr<ChatThreadData>> _chat_map;
    QMap<int, int> _uid_to_thread_id;
    QVector<int> _chat_thread_ids;
    QMap<QString, std::shared_ptr<FileInfo>> _name_files;
    QMap<QString, std::shared_ptr<FileInfo>> _file_infos;
    QMap<QString, std::shared_ptr<DownloadInfo>> _name_to_download_info;
    QSet<QString> _avatar_requested;
    QMap<QString, QList<QLabel*>> _name_to_reset_labels;
    QMap<QString, std::shared_ptr<MsgInfo>> _file_msgs;
};

#endif //MPTCHAT_USERMGR_H

