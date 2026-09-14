//
// Created by mpt on 2026/7/1.
//

#ifndef GETSERVER_REDISMGR_H
#define GETSERVER_REDISMGR_H

#include "Singleton.h"
#include <sw/redis++/redis++.h>
#include <string>
#include <chrono>
#include "ConfigMgr.h"
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

private:
    RedisMgr() = default;

    std::unique_ptr<sw::redis::Redis> _redis;
};

class RedisPool : public Singleton<RedisPool> {
public:
    RedisPool(RedisPool&) = delete;
    RedisPool operator=(RedisPool&) = delete;
private:
    std::unique_ptr<RedisMgr> _redisMgr;

};


#endif //GETSERVER_REDISMGR_H