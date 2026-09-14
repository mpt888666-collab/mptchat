//
// Created by mpt on 2026/7/1.
//

#include "RedisMgr.h"
#include <cstring>


RedisMgr::~RedisMgr()
{
    Close();
}

bool RedisMgr::Connect()
{
    Close();
    try
    {
        // 1. 基础连接配置（host/port/db/password 写在这里）
        sw::redis::ConnectionOptions conn_opts;
        std::string host = ConfigMgr::Inst()["RedisServer"]["Host"];
        int port = std::stoi(ConfigMgr::Inst()["RedisServer"]["Port"]);
        int db = std::stoi(ConfigMgr::Inst()["RedisServer"]["Db"]);
        int size = std::stoi(ConfigMgr::Inst()["RedisServer"]["Size"]);
        conn_opts.host = host;
        conn_opts.port = port;
        conn_opts.db = db;

        // 2. 连接池配置（只写池大小）
        sw::redis::ConnectionPoolOptions pool_opts;
        pool_opts.size = size;

        // 3. Redis 内部自动创建连接池，不需要手动 new ConnectionPool
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