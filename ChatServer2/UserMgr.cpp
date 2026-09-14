//
// Created by mpt on 2026/7/26.
//

#include "UserMgr.h"
#include <utility>

#include "CSession.h"

std::shared_ptr<CSession> UserMgr::getSession(const int uid) {
    std::lock_guard<std::mutex> lock(_session_mtx);
    auto it = _uid_to_session.find(uid);
    if (it == _uid_to_session.end()) {
        return nullptr;
    }
    return it->second;
}

void UserMgr::setSession(const int uid, std::shared_ptr<CSession> Session) {
    std::lock_guard<std::mutex> lock(_session_mtx);
    _uid_to_session[uid] = Session;
}

void UserMgr::removeSession(const int uid, std::string session_id) {
    std::lock_guard<std::mutex> lock(_session_mtx);
    auto iter = _uid_to_session.find(uid);
    if (iter != _uid_to_session.end()) {
        auto session_id_ = iter->second->GetSessionId();
        if (session_id_ != session_id) {
            return;
        }
        _uid_to_session.erase(iter);
    }
}

UserMgr:: ~UserMgr() {
    _uid_to_session.clear();
}


