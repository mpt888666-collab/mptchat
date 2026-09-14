//
// Created by mpt on 2026/7/26.
//

#ifndef CHATSERVER_USERMGR_H
#define CHATSERVER_USERMGR_H
#include "Singleton.h"
#include <unordered_map>
class CSession;
class UserMgr : public Singleton<UserMgr>{
    friend class Singleton<UserMgr>;
public:
    std::shared_ptr<CSession> getSession(int uid);

    void setSession(int uid, std::shared_ptr<CSession> Session);

    ~UserMgr();

    void removeSession(int uid, std::string session_id);
private:
    std::mutex _session_mtx;
    std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
    UserMgr() = default;
};


#endif //CHATSERVER_USERMGR_H