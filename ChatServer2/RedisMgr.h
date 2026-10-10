//
// Created by mpt on 2026/7/1.
//

#ifndef GETSERVER_REDISMGR_H
#define GETSERVER_REDISMGR_H

#include "Singleton.h"
#include <sw/redis++/redis++.h>
#include <string>
#include <chrono>
#include <thread>
#include "ConfigMgr.h"
#include <memory>

class RedisMgr : public Singleton<RedisMgr>
{
friend class Singleton<RedisMgr>;
public:
    ~RedisMgr();

    bool Connect();
    bool Get(const std::string& key, std::string& value);
    bool Set(const std::string& key, const std::string& value);
    bool Auth(const std::string& password);

    bool LPush(const std::string& key, const std::string& value);
    bool LPop(const std::string& key, std::string& value);
    bool RPush(const std::string& key, const std::string& value);
    bool RPop(const std::string& key, std::string& value);

    bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
    bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
    std::string HGet(const std::string& key, const std::string& hkey);

    bool Del(const std::string& key);
    bool ExistsKey(const std::string& key);

    void Close();

    long long HDel(const std::string &key, const std::string &hkey);
    long long HDel(const char *key, const char *hkey);
    long long HIncr(const std::string &key, const std::string &hkey);
    long long HIncr(const char *key, const char *hkey);
    long long HDecr(const std::string &key, const std::string &hkey);

    // ====== Distributed Lock ======
    // Try to acquire lock once (non-blocking). Returns true on success.
    // key: lock name, value: unique lock owner id, expire_time: seconds
    bool Lock(const std::string &key, const std::string &value, int expire_time);

    // Release lock. Only releases if value matches (prevents releasing others' locks).
    bool Unlock(const std::string &key, const std::string &value);

    // Try to acquire lock with retries (blocking). Returns true if acquired within timeout.
    // retry_interval_ms: wait between attempts, timeout_ms: total wait limit
    bool TryLock(const std::string &key, const std::string &value,
                 int expire_time, int retry_interval_ms = 100, int timeout_ms = 5000);

private:
    RedisMgr() = default;

    std::unique_ptr<sw::redis::Redis> _redis;
};

// RAII lock guard — auto-releases on destruction
class RedisLockGuard {
public:
    RedisLockGuard(const std::string &key, const std::string &value, int expire_time)
        : _key(key), _value(value), _locked(false) {
        _locked = RedisMgr::GetInstance()->Lock(key, value, expire_time);
    }

    RedisLockGuard(const std::string &key, const std::string &value,
                   int expire_time, int retry_interval_ms, int timeout_ms)
        : _key(key), _value(value), _locked(false) {
        _locked = RedisMgr::GetInstance()->TryLock(key, value, expire_time,
                                                     retry_interval_ms, timeout_ms);
    }

    ~RedisLockGuard() {
        if (_locked) {
            RedisMgr::GetInstance()->Unlock(_key, _value);
        }
    }

    bool IsLocked() const { return _locked; }

    // Non-copyable, non-movable
    RedisLockGuard(const RedisLockGuard&) = delete;
    RedisLockGuard& operator=(const RedisLockGuard&) = delete;

private:
    std::string _key;
    std::string _value;
    bool _locked;
};


#endif //GETSERVER_REDISMGR_H
