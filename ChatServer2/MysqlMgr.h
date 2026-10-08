//
// Created by mpt on 2026/7/3.
//

#ifndef GETSERVER_MYSQLMGR_H
#define GETSERVER_MYSQLMGR_H
#include <complex.h>
#include <vector>
#include <boost/mpl/size.hpp>
#include <boost/range/size.hpp>
#include <mysql/mysql.h>
#include "const.h"
#include "data.h"

// struct UserInfo {
//     int uid;
//     std::string username;
//     std::string password;
//     std::string email;
//     std::string nick;
//     std::string desc;
//     std::string icon;
//     int sex;
// };
struct UserInfo;
struct MysqlConnDeleter {
    void operator()(MYSQL* conn) const {
        if (conn) {
            mysql_close(conn);
        }
    }
};
using MysqlConnUPtr = std::unique_ptr<MYSQL, MysqlConnDeleter>;

class MySQLPool{
public:
    MySQLPool(MySQLPool&) = delete;

    MySQLPool operator=(MySQLPool&) = delete;

    MySQLPool(std::string host, std::string user, std::string password,
        std::string dbname, unsigned int port, size_t size = 2): _poolSize(size), _host(host),
    _user(user), _pwd(password),_db(dbname), _port(port){
        int current_conn_count = 0;
        for (size_t i = 0; i < _poolSize; i++) {
            auto conn = CreateOneConn();
            if (!conn) {
                std::cerr << "Create is " << i << "sql failed!" << std::endl;
            }else {
                _mysqls.push(std::move(conn));
            }
        }
        size_t valid = _mysqls.size();
        if (valid == 0)  throw std::runtime_error("mysql pool create zero valid connections");
    }

    MysqlConnUPtr GetConnection(int timeoutMs = 3000) {
        std::unique_lock<std::mutex> lock(_mtx);
        // 等待空闲连接或停止信号
        bool hasIdle = _cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
            return !_mysqls.empty() || _b_stop;
        });

        if (_b_stop || !hasIdle) {
            return nullptr;
        }

        MysqlConnUPtr conn = std::move(_mysqls.front());
        _mysqls.pop();
        lock.unlock();

        // 检测连接是否失效，失效重建
        if (mysql_ping(conn.get()) != 0) {
            std::cerr << "sql conn is down , plase reset connection" << std::endl;
            conn = CreateOneConn();
            // 重建失败直接返回空
            if (!conn) {
                std::cerr << "sql reset connection failed " << std::endl;
                return nullptr;
            }
        }
        return conn;
    }

    void ReturnConnection(MysqlConnUPtr conn) {
        if (!conn) return;
        std::unique_lock<std::mutex> lock(_mtx);
        if (_b_stop) return;

        // 重置会话：回滚事务、清空会话状态
        mysql_reset_connection(conn.get());
        _mysqls.push(std::move(conn));
        _cv.notify_one();
    }

    void Stop() {
        std::unique_lock<std::mutex> lock(_mtx);
        if (_b_stop) return;
        _b_stop = true;
        lock.unlock();
        _cv.notify_all();
    }
    ~MySQLPool() {
        Stop();
        ClearAllConn();
    }

private:

    MysqlConnUPtr CreateOneConn() {
        MYSQL* raw = mysql_init(nullptr);
        if (!raw) {
            std::cerr << "MySQL init error\n";
            return nullptr;
        }
        MysqlConnUPtr conn(raw, MysqlConnDeleter{});
         bool ret = mysql_real_connect(
            raw,
            _host.c_str(),
            _user.c_str(),
            _pwd.c_str(),
            _db.c_str(),
            _port,
            nullptr,
            0
        );
        if (!ret) {
            std::cerr << "set sql failed!" << mysql_error(raw) << std::endl;
            return nullptr;
        }
        // 设置utf8mb4，支持中文emoji
        mysql_set_character_set(raw, "utf8mb4");
        return conn;
    }

    void ClearAllConn() {
        std::unique_lock<std::mutex> lock(_mtx);
        while (!_mysqls.empty()) {
            _mysqls.pop();
        }
    }

    std::queue<MysqlConnUPtr> _mysqls;
    size_t _poolSize;
    std::mutex _mtx;
    std::condition_variable _cv;
    bool _b_stop = false;
    const std::string _host, _user, _pwd, _db;
    const unsigned int _port;
};


class MysqlMgr : public Singleton<MysqlMgr> {
    friend class Singleton<MysqlMgr>;
public:
    MysqlConnUPtr GetConn() {
        if (!_pool) return nullptr;
        return _pool->GetConnection();
    }

    void ReturnConn(MysqlConnUPtr conn) {
        if (_pool)
            _pool->ReturnConnection(std::move(conn));
    }

    void StopPool() {
        if (_pool) {
            _pool->Stop();
            _pool.reset();
        }
    }

    bool IsUserExist(const std::string& username);

    bool IsEmailUsed(const std::string& email);

    bool RegisterUser(const std::string& username, const std::string& password, const std::string& email);

    bool ModifyPasswordByUserEmail(const std::string& username, const std::string& email, const std::string& newPwd);

    // 登录校验：同时匹配 username + email + password，存在返回true
    bool CheckUserLogin(const std::string& username, const std::string& email, const std::string& password);

    bool CheckUserLogin(const std::string& username, const std::string& email, const std::string& password, UserInfo& userInfo);

    std::shared_ptr<UserInfo> GetUserInfoById(int id);

    std::shared_ptr<UserInfo> GetUserInfoByName(const std::string& username);

    bool AddFriendApply(int from_uid, int to_uid);

    bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>> &apply_list);
    bool GetFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friend_list);

    bool GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
                        std::vector<std::shared_ptr<ChatThreadInfo>> &threads,
                        bool &loadMore, int &nextLastId);

    bool CreatePrivateChat(int user1_id, int user2_id, int &thread_id);

    bool AddChatMsg(std::vector<std::shared_ptr<ChatMessage>> &chat_datas);
    bool AddChatMsg(std::shared_ptr<ChatMessage> chat_msg);

    std::shared_ptr<PageResult> LoadChatMsg(int thread_id, int message_id, int page_size);
    bool IsGroupThread(int thread_id);
    bool GetGroupMembers(int thread_id, std::vector<int>& members);

    bool UpdateChatMsgStatus(int message_id, int status);

    bool UpdateHeadInfo(int uid, const std::string &icon);

    bool AuthFriendApply(int from, int to);

    bool AddFriend(int me_uid, int peer_uid, const std::string &back_name);

    bool CreateGroupChat(uint64_t host_uid, const std::vector<int>& members, int& thread_id);


private:
    MysqlMgr() {
        try {
            auto& cfgMgr = ConfigMgr::Inst();
            std::string host = cfgMgr["MySQLServer"]["Host"];
            std::string user = cfgMgr["MySQLServer"]["User"];
            std::string pwd = cfgMgr["MySQLServer"]["Password"];
            std::string db = cfgMgr["MySQLServer"]["Db"];
            unsigned int port = static_cast<unsigned int>(std::stoi(cfgMgr["MySQLServer"]["Port"]));
            size_t pool_size = static_cast<size_t>(std::stoi(cfgMgr["MySQLServer"]["Pool_size"]));
            _pool = std::make_unique<MySQLPool>(host, user, pwd, db, port, pool_size);
        } catch (std::exception& e) {
            std::cerr << "[MysqlMgr] init failed:" << e.what() << std::endl;
        }
    }
    bool CheckUserLoginImpl(const std::string& username,const std::string& email,const std::string& password,UserInfo* outInfo);


    std::unique_ptr<MySQLPool> _pool;
};


struct MysqlConnGuard {
    explicit MysqlConnGuard(MysqlConnUPtr c) : _conn(std::move(c)) {}
    ~MysqlConnGuard() {
        if (_conn)
            MysqlMgr::GetInstance()->ReturnConn(std::move(_conn));
    }
    MysqlConnUPtr& Get() { return _conn; }
    MYSQL* Raw() { return _conn.get(); }
    MysqlConnUPtr _conn;
};

#endif //GETSERVER_MYSQLMGR_H
