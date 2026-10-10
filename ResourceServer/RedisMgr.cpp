//
// Created by mpt on 2026/7/1.
//

#include "RedisMgr.h"
#include <cstring>
#include "LogicSystem.h"
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
RedisMgr::~RedisMgr()
{
    Close();
}

bool RedisMgr::Connect()
{
    Close();
    try
    {
        sw::redis::ConnectionOptions conn_opts;
        std::string host = ConfigMgr::Inst()["RedisServer"]["Host"];
        int port = std::stoi(ConfigMgr::Inst()["RedisServer"]["Port"]);
        int db = std::stoi(ConfigMgr::Inst()["RedisServer"]["Db"]);
        int size = std::stoi(ConfigMgr::Inst()["RedisServer"]["Size"]);
        conn_opts.host = host;
        conn_opts.port = port;
        conn_opts.db = db;

        sw::redis::ConnectionPoolOptions pool_opts;
        pool_opts.size = size;

        _redis = std::make_unique<sw::redis::Redis>(conn_opts, pool_opts);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::Auth(const std::string& password)
{
    if (!_redis) return false;
    try
    {
        _redis->auth(password);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
    if (!_redis) return false;
    try
    {
        auto res = _redis->get(key);
        if (!res) return false;
        value = *res;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::Set(const std::string& key, const std::string& value)
{
    if (!_redis) return false;
    try
    {
        _redis->set(key, value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
    if (!_redis) return false;
    try
    {
        _redis->lpush(key, value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::LPop(const std::string& key, std::string& value)
{
    if (!_redis) return false;
    try
    {
        auto res = _redis->lpop(key);
        if (!res) return false;
        value = *res;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::RPush(const std::string& key, const std::string& value)
{
    if (!_redis) return false;
    try
    {
        _redis->rpush(key, value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::RPop(const std::string& key, std::string& value)
{
    if (!_redis) return false;
    try
    {
        auto res = _redis->rpop(key);
        if (!res) return false;
        value = *res;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value)
{
    if (!_redis) return false;
    try
    {
        _redis->hset(key, hkey, value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
    if (!_redis || !key || !hkey || !hvalue)
        return false;

    std::string val(hvalue, hvaluelen);
    try
    {
        _redis->hset(std::string(key), std::string(hkey), val);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
    if (!_redis) return "";
    try
    {
        auto res = _redis->hget(key, hkey);
        return res ? *res : "";
    }
    catch (...)
    {
        return "";
    }
}

bool RedisMgr::Del(const std::string& key)
{
    if (!_redis) return false;
    try
    {
        _redis->del(key);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::ExistsKey(const std::string& key)
{
    if (!_redis) return false;
    try
    {
        return _redis->exists(key) > 0;
    }
    catch (...)
    {
        return false;
    }
}

void RedisMgr::Close()
{
    _redis.reset();
}

long long RedisMgr::HDel(const std::string& key, const std::string& hkey)
{
    if (!_redis)
        return -1;
    try
    {
        long long cnt = _redis->hdel(key, hkey);
        return cnt;
    }
    catch (...)
    {
        return -1;
    }
}

long long RedisMgr::HDel(const char* key, const char* hkey)
{
    if (!_redis || !key || !hkey)
        return -1;
    try
    {
        long long cnt = _redis->hdel(std::string(key), std::string(hkey));
        return cnt;
    }
    catch (...)
    {
        return -1;
    }
}

long long RedisMgr::HIncr(const std::string& key, const std::string& hkey)
{
    if (!_redis)
        return -1;
    try
    {
        return _redis->hincrby(key, hkey, 1);
    }
    catch (...)
    {
        return -1;
    }
}

long long RedisMgr::HIncr(const char* key, const char* hkey)
{
    if (!_redis || !key || !hkey)
        return -1;
    try
    {
        return _redis->hincrby(std::string(key), std::string(hkey), 1);
    }
    catch (...)
    {
        return -1;
    }
}

// ===================================================================
//  Distributed Lock
// ===================================================================

// Lua: SET key value NX EX expire -- atomic acquire
static const std::string LOCK_SCRIPT = R"(
local ret = redis.call('SET', KEYS[1], ARGV[1], 'NX', 'EX', tonumber(ARGV[2]))
return ret == nil and 0 or 1
)";

// Lua: GET + compare + DEL -- atomic release
static const std::string UNLOCK_SCRIPT = R"(
if redis.call('GET', KEYS[1]) == ARGV[1] then
    return redis.call('DEL', KEYS[1])
else
    return 0
end
)";

bool RedisMgr::Lock(const std::string& key, const std::string& value, int expire_time)
{
    if (!_redis) return false;
    try
    {
        auto res = _redis->eval<long long>(LOCK_SCRIPT,
            std::initializer_list<sw::redis::StringView>{key},
            std::initializer_list<sw::redis::StringView>{value, std::to_string(expire_time)});
        return res == 1;
    }
    catch (...)
    {
        return false;
    }
}

bool RedisMgr::Unlock(const std::string& key, const std::string& value)
{
    if (!_redis) return false;
    try
    {
        auto res = _redis->eval<long long>(UNLOCK_SCRIPT,
            std::initializer_list<sw::redis::StringView>{key},
            std::initializer_list<sw::redis::StringView>{value});
        return res == 1;
    }
    catch (...)
    {
        return false;
    }
}

long long RedisMgr::HDecr(const std::string& key, const std::string& hkey) {
    if (!_redis) return -1;
    try { return _redis->hincrby(key, hkey, -1); }
    catch (...) { return -1; }
}

bool RedisMgr::TryLock(const std::string& key, const std::string& value,
                        int expire_time, int retry_interval_ms, int timeout_ms)
{
    auto start = std::chrono::steady_clock::now();

    while (true)
    {
        if (Lock(key, value, expire_time))
            return true;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        if (elapsed >= timeout_ms)
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(retry_interval_ms));
    }
}

bool RedisMgr::SetExp(const std::string& key, const std::string& value, int expire_seconds)
{
    if (!_redis)
        return false;
    try
    {
        _redis->set(key, value, std::chrono::seconds(expire_seconds));
        std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
            << " " << value << " ] success ! " << std::endl;
        return true;
    }
    catch (...)
    {
        std::cout << "Execute command [ SETEX " << key << " " << expire_seconds
            << " " << value << " ] failure ! " << std::endl;
        return false;
    }
}

bool RedisMgr::SetFileInfo(const std::string& md5, std::shared_ptr<FileInfo> file_info)
{
    json root;
    root["file_path_str"] = file_info->_file_path_str;
    root["name"] = file_info->_name;
    root["seq"] = file_info->_seq;
    root["total_size"] = file_info->_total_size;
    root["trans_size"] = file_info->_trans_size;

    std::string file_info_str = root.dump();
    std::string redis_key = "file_upload_" + md5;

    bool success = SetExp(redis_key, file_info_str, 3600);
    return success;
}


std::shared_ptr<FileInfo> RedisMgr::GetFileInfo(const std::string& md5)
{
    if (!_redis)
        return nullptr;

    std::string redis_key = "file_upload_" + md5;
    std::string json_str;

    // 从 Redis 里取出 JSON 串
    bool ok = Get(redis_key, json_str);
    if (!ok)
    {
        return nullptr;
    }

    json root;
    json reader = json::parse(json_str);   

    auto info = std::make_shared<FileInfo>();
    info->_file_path_str = reader["file_path_str"].get<std::string>();
    info->_name = reader["name"].get<std::string>();
    info->_seq = reader["seq"].get<int>();
    info->_total_size = reader["total_size"].get<long long>();
    info->_trans_size = reader["trans_size"].get<long long>();

    return info;
}






