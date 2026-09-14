//
// Created by mpt on 2026/7/3.
//

#include "MysqlMgr.h"
#include "data.h"
#include <iostream>
#include <algorithm>
bool MysqlMgr::IsUserExist(const std::string& username)
    {
        auto conn = GetConn();
        MYSQL* mysql = conn.get();
        if (!mysql)
        {
            std::cerr << "Get mysql connection failed when check user exist" << std::endl;
            return false;
        }

        const std::string sql = "SELECT COUNT(1) FROM `user` WHERE username = ?";
        MYSQL_STMT* stmt = mysql_stmt_init(mysql);
        if (!stmt)
        {
            std::cerr << "mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
            return false;
        }

        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "stmt prepare failed: " << mysql_stmt_error(stmt) << std::endl;
            mysql_stmt_close(stmt);
            return false;
        }

        // 绑定输入参数 username
        MYSQL_BIND inputBind[1] = {0};
        char nameBuf[64] = {0};
        strncpy(nameBuf, username.c_str(), sizeof(nameBuf) - 1);
        unsigned long nameLen = username.size();

        inputBind[0].buffer_type = MYSQL_TYPE_STRING;
        inputBind[0].buffer = nameBuf;
        inputBind[0].buffer_length = sizeof(nameBuf);
        inputBind[0].length = &nameLen;
        mysql_stmt_bind_param(stmt, inputBind);

        // 执行sql
        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "stmt execute failed: " << mysql_stmt_error(stmt) << std::endl;
            mysql_stmt_close(stmt);
            return false;
        }

        // 绑定输出 count 数值
        MYSQL_BIND outputBind[1] = {0};
        long long userCount = 0;
        outputBind[0].buffer_type = MYSQL_TYPE_LONGLONG;
        outputBind[0].buffer = &userCount;
        mysql_stmt_bind_result(stmt, outputBind);

        bool exist = false;
        if (mysql_stmt_fetch(stmt) == 0)
        {
            exist = (userCount > 0);
        }

        // 释放stmt资源
        mysql_stmt_free_result(stmt);
        mysql_stmt_close(stmt);
        ReturnConn(std::move(conn));
        return exist;
    }

bool MysqlMgr::IsEmailUsed(const std::string& email)
{
    auto conn = GetConn();
    MYSQL* mysql = conn.get();
    if (!mysql)
    {
        std::cerr << "Get mysql connection failed when check email" << std::endl;
        return false;
    }

    const std::string sql = "SELECT COUNT(1) FROM `user` WHERE email = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        return false;
    }

    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
    {
        std::cerr << "stmt prepare failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND inputBind[1] = {0};
    char buf[100] = {0};
    strncpy(buf, email.c_str(), sizeof(buf) - 1);
    unsigned long len = email.size();
    inputBind[0].buffer_type = MYSQL_TYPE_STRING;
    inputBind[0].buffer = buf;
    inputBind[0].buffer_length = sizeof(buf);
    inputBind[0].length = &len;
    mysql_stmt_bind_param(stmt, inputBind);

    if (mysql_stmt_execute(stmt) != 0)
    {
        std::cerr << "stmt execute failed: " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND outputBind[1] = {0};
    long long count = 0;
    outputBind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    outputBind[0].buffer = &count;
    mysql_stmt_bind_result(stmt, outputBind);

    bool used = (mysql_stmt_fetch(stmt) == 0 && count > 0);
    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    ReturnConn(std::move(conn));
    return used;
}

// 注册新用户，传入用户名、密码、邮箱
// 返回true：插入成功；返回false：用户名/邮箱重复、数据库异常、连接失败
bool MysqlMgr::RegisterUser(const std::string& username, const std::string& password, const std::string& email)
{
    // 1. 前置校验：用户名、邮箱不能重复
    if (IsUserExist(username))
    {
        std::cerr << "Register failed: username already exists" << std::endl;
        return false;
    }
    if (IsEmailUsed(email))
    {
        std::cerr << "Register failed: email already used" << std::endl;
        return false;
    }

    // 2. 获取数据库连接
    auto conn = GetConn();
    MYSQL* mysql = conn.get();
    if (!mysql)
    {
        std::cerr << "Register failed: get mysql connection error" << std::endl;
        return false;
    }

    // 3. 插入SQL，三个占位符对应 username password email
    const std::string insertSql = "INSERT INTO `user`(username, password, email) VALUES (?,?,?)";
    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "Register failed: stmt init error " << mysql_error(mysql) << std::endl;
        return false;
    }

    // 4. 预编译SQL模板
    if (mysql_stmt_prepare(stmt, insertSql.c_str(), insertSql.size()) != 0)
    {
        std::cerr << "Register failed: prepare sql error " << mysql_stmt_error(stmt) << std::endl;
        mysql_stmt_close(stmt);
        return false;
    }

    // 5. 准备缓冲区，绑定三个字符串参数
    MYSQL_BIND bindParams[3] = {0};
    char userBuf[64] = {0};
    char pwdBuf[100] = {0};
    char emailBuf[100] = {0};

    unsigned long userLen = username.size();
    unsigned long pwdLen = password.size();
    unsigned long emailLen = email.size();

    strncpy(userBuf, username.c_str(), sizeof(userBuf) - 1);
    strncpy(pwdBuf, password.c_str(), sizeof(pwdBuf) - 1);
    strncpy(emailBuf, email.c_str(), sizeof(emailBuf) - 1);

    // 绑定第一个参数：用户名
    bindParams[0].buffer_type = MYSQL_TYPE_STRING;
    bindParams[0].buffer = userBuf;
    bindParams[0].buffer_length = sizeof(userBuf);
    bindParams[0].length = &userLen;

    // 绑定第二个参数：密码
    bindParams[1].buffer_type = MYSQL_TYPE_STRING;
    bindParams[1].buffer = pwdBuf;
    bindParams[1].buffer_length = sizeof(pwdBuf);
    bindParams[1].length = &pwdLen;

    // 绑定第三个参数：邮箱
    bindParams[2].buffer_type = MYSQL_TYPE_STRING;
    bindParams[2].buffer = emailBuf;
    bindParams[2].buffer_length = sizeof(emailBuf);
    bindParams[2].length = &emailLen;

    mysql_stmt_bind_param(stmt, bindParams);

    // 6. 执行插入语句
    bool insertSuccess = true;
    if (mysql_stmt_execute(stmt) != 0)
    {
        std::cerr << "Register failed: execute insert sql error " << mysql_stmt_error(stmt) << std::endl;
        insertSuccess = false;
    }

    // 7. 释放预处理资源，防止内存泄漏
    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    ReturnConn(std::move(conn));
    return insertSuccess;
}

bool MysqlMgr::ModifyPasswordByUserEmail(const std::string& username, const std::string& email, const std::string& newPwd)
{
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[ModifyPasswordByUserEmail] get mysql connection failed" << std::endl;
        return false;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    // 预处理SQL：3个占位符 newPwd / username / email
    const std::string sql = "UPDATE user SET password=? WHERE username=? AND email=?";
    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[ModifyPasswordByUserEmail] stmt init error: " << mysql_error(mysql) << std::endl;
        return false;
    }

    bool ok = true;
    do
    {
        // 预处理语句
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[ModifyPasswordByUserEmail] prepare error: " << mysql_stmt_error(stmt) << std::endl;
            ok = false;
            break;
        }

        // 绑定3个字符串参数
        MYSQL_BIND bind[3]{};
        // 1. new password
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)newPwd.data();
        bind[0].buffer_length = newPwd.size();

        // 2. username
        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)username.data();
        bind[1].buffer_length = username.size();

        // 3. email
        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = (void*)email.data();
        bind[2].buffer_length = email.size();

        if (mysql_stmt_bind_param(stmt, bind) != 0)
        {
            std::cerr << "[ModifyPasswordByUserEmail] bind param error: " << mysql_stmt_error(stmt) << std::endl;
            ok = false;
            break;
        }

        // 执行更新
        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[ModifyPasswordByUserEmail] execute error: " << mysql_stmt_error(stmt) << std::endl;
            ok = false;
            break;
        }

        // 影响行数为0 → 用户名+邮箱不匹配，无此用户
        my_ulonglong affected = mysql_stmt_affected_rows(stmt);
        if (affected == 0)
        {
            std::cerr << "[ModifyPasswordByUserEmail] username + email mismatch, no user matched" << std::endl;
            ok = false;
        }

    } while (false);

    mysql_stmt_close(stmt);
    return ok;
}

bool MysqlMgr::CheckUserLogin(const std::string& username, const std::string& email, const std::string& password){
    return CheckUserLoginImpl(username, email, password, nullptr);
}

bool MysqlMgr::CheckUserLogin(const std::string& username, const std::string& email, const std::string& password, UserInfo& userInfo){
    return CheckUserLoginImpl(username, email, password, &userInfo);
}

bool MysqlMgr::CheckUserLoginImpl(const std::string& username,
                                   const std::string& email,
                                   const std::string& password,
                                   UserInfo* outInfo)
{
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[CheckUserLogin] get mysql conn failed" << std::endl;
        return false;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    const std::string sql = "SELECT id,username,email,password FROM user WHERE username=? AND email=? AND password=? LIMIT 1";
    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[CheckUserLogin] stmt init err: " << mysql_error(mysql) << std::endl;
        return false;
    }

    bool succ = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[CheckUserLogin] prepare err: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        MYSQL_BIND bindIn[3]{};
        bindIn[0].buffer_type = MYSQL_TYPE_STRING;
        bindIn[0].buffer = (void*)username.data();
        bindIn[0].buffer_length = username.size();

        bindIn[1].buffer_type = MYSQL_TYPE_STRING;
        bindIn[1].buffer = (void*)email.data();
        bindIn[1].buffer_length = email.size();

        bindIn[2].buffer_type = MYSQL_TYPE_STRING;
        bindIn[2].buffer = (void*)password.data();
        bindIn[2].buffer_length = password.size();

        if (mysql_stmt_bind_param(stmt, bindIn) != 0)
        {
            std::cerr << "[CheckUserLogin] bind param err: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[CheckUserLogin] execute err: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // ========== 缺失的关键一行 ==========
        mysql_stmt_store_result(stmt);
        // ===================================

        my_ulonglong rows = mysql_stmt_num_rows(stmt);
        if (rows == 0)
            break;

        succ = true;
        if (outInfo != nullptr)
        {
            MYSQL_BIND bindOut[4]{};
            long long out_uid = 0;
            char buf_name[64]{};
            char buf_email[100]{};
            char buf_pwd[100]{};
            unsigned long len_name, len_email, len_pwd;

            bindOut[0].buffer_type = MYSQL_TYPE_LONGLONG;
            bindOut[0].buffer = &out_uid;

            bindOut[1].buffer_type = MYSQL_TYPE_STRING;
            bindOut[1].buffer = buf_name;
            bindOut[1].buffer_length = sizeof(buf_name);
            bindOut[1].length = &len_name;

            bindOut[2].buffer_type = MYSQL_TYPE_STRING;
            bindOut[2].buffer = buf_email;
            bindOut[2].buffer_length = sizeof(buf_email);
            bindOut[2].length = &len_email;

            bindOut[3].buffer_type = MYSQL_TYPE_STRING;
            bindOut[3].buffer = buf_pwd;
            bindOut[3].buffer_length = sizeof(buf_pwd);
            bindOut[3].length = &len_pwd;

            mysql_stmt_bind_result(stmt, bindOut);
            mysql_stmt_fetch(stmt);

            outInfo->uid = static_cast<int>(out_uid);
            outInfo->username.assign(buf_name, len_name);
            outInfo->email.assign(buf_email, len_email);
            outInfo->password.assign(buf_pwd, len_pwd);
        }
    } while (0);

    mysql_stmt_close(stmt);
    return succ;
}

std::shared_ptr<UserInfo> MysqlMgr::GetUserInfoById(int id)
{
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[GetUserInfoById] get mysql connection failed" << std::endl;
        return std::shared_ptr<UserInfo>(nullptr);
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    // 严格按数据库字段顺序书写 SELECT
    const std::string sql = R"(
        SELECT id,username,password,email,nick,`desc`,icon,sex
        FROM `user` WHERE id = ? LIMIT 1
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[GetUserInfoById] stmt init error: " << mysql_error(mysql) << std::endl;
        return nullptr;
    }

    std::shared_ptr<UserInfo> res = nullptr;
    do
    {
        // 预编译
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[GetUserInfoById] prepare failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 绑定输入：查询条件 id
        MYSQL_BIND bindInput[1]{};
        long long input_id = id;
        bindInput[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[0].buffer = &input_id;
        if (mysql_stmt_bind_param(stmt, bindInput) != 0)
        {
            std::cerr << "[GetUserInfoById] bind param failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 执行SQL
        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[GetUserInfoById] execute failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 缓存结果，用于判断行数
        mysql_stmt_store_result(stmt);
        my_ulonglong rowCnt = mysql_stmt_num_rows(stmt);
        if (rowCnt == 0)
        {
            std::cerr << "[GetUserInfoById] no user found, id = " << id << std::endl;
            break;
        }

        // 创建智能指针对象
        res = std::make_shared<UserInfo>();

        // 输出绑定缓冲区，和SELECT字段一一对应 8个字段
        MYSQL_BIND bindOutput[8]{};

        char buf_username[64]{};
        char buf_password[128]{};
        char buf_email[128]{};
        char buf_nick[64]{};
        char buf_desc[512]{};
        char buf_icon[256]{};

        unsigned long len_username, len_password, len_email, len_nick, len_desc, len_icon;
        long long out_id = 0;
        int out_sex = 0;

        // 1 id
        bindOutput[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindOutput[0].buffer = &out_id;

        // 2 username
        bindOutput[1].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[1].buffer = buf_username;
        bindOutput[1].buffer_length = sizeof(buf_username);
        bindOutput[1].length = &len_username;

        // 3 password
        bindOutput[2].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[2].buffer = buf_password;
        bindOutput[2].buffer_length = sizeof(buf_password);
        bindOutput[2].length = &len_password;

        // 4 email
        bindOutput[3].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[3].buffer = buf_email;
        bindOutput[3].buffer_length = sizeof(buf_email);
        bindOutput[3].length = &len_email;

        // 5 nick
        bindOutput[4].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[4].buffer = buf_nick;
        bindOutput[4].buffer_length = sizeof(buf_nick);
        bindOutput[4].length = &len_nick;

        // 6 desc（关键字带反引号）
        bindOutput[5].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[5].buffer = buf_desc;
        bindOutput[5].buffer_length = sizeof(buf_desc);
        bindOutput[5].length = &len_desc;

        // 7 icon
        bindOutput[6].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[6].buffer = buf_icon;
        bindOutput[6].buffer_length = sizeof(buf_icon);
        bindOutput[6].length = &len_icon;

        // 8 sex
        bindOutput[7].buffer_type = MYSQL_TYPE_LONG;
        bindOutput[7].buffer = &out_sex;

        mysql_stmt_bind_result(stmt, bindOutput);

        // 拉取一行数据
        if (mysql_stmt_fetch(stmt) != 0)
        {
            std::cerr << "[GetUserInfoById] fetch data error: " << mysql_stmt_error(stmt) << std::endl;
            res.reset();
            break;
        }

        // 映射赋值到UserInfo
        res->uid = static_cast<int>(out_id);
        res->username.assign(buf_username, len_username);
        res->password.assign(buf_password, len_password);
        res->email.assign(buf_email, len_email);
        res->nick.assign(buf_nick, len_nick);
        res->desc.assign(buf_desc, len_desc);
        res->icon.assign(buf_icon, len_icon);
        res->sex = out_sex;

    } while (false);

    // 统一释放stmt资源
    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    return res;
}

std::shared_ptr<UserInfo> MysqlMgr::GetUserInfoByName(const std::string& username)
{
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[GetUserInfoByName] get mysql connection failed" << std::endl;
        return nullptr;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    // 查询字段顺序必须和输出绑定数组严格一一对应
    const std::string sql = R"(
        SELECT id,username,password,email,nick,`desc`,icon,sex
        FROM `user` WHERE username = ? LIMIT 1
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[GetUserInfoByName] stmt init error: " << mysql_error(mysql) << std::endl;
        return nullptr;
    }

    std::shared_ptr<UserInfo> res = nullptr;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[GetUserInfoByName] prepare failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 绑定输入：username字符串
        MYSQL_BIND bindInput[1]{};
        char nameBuf[64]{};
        unsigned long nameLen = username.size();
        strncpy(nameBuf, username.c_str(), sizeof(nameBuf) - 1);

        bindInput[0].buffer_type = MYSQL_TYPE_STRING;
        bindInput[0].buffer = nameBuf;
        bindInput[0].buffer_length = sizeof(nameBuf);
        bindInput[0].length = &nameLen;

        if (mysql_stmt_bind_param(stmt, bindInput) != 0)
        {
            std::cerr << "[GetUserInfoByName] bind param failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[GetUserInfoByName] execute failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        mysql_stmt_store_result(stmt);
        my_ulonglong rowCnt = mysql_stmt_num_rows(stmt);
        if (rowCnt == 0)
        {
            std::cerr << "[GetUserInfoByName] no user found, username = " << username << std::endl;
            break;
        }

        res = std::make_shared<UserInfo>();

        // 输出绑定，和SELECT 8个字段顺序完全匹配
        MYSQL_BIND bindOutput[8]{};

        char buf_username[64]{};
        char buf_password[128]{};
        char buf_email[128]{};
        char buf_nick[64]{};
        char buf_desc[512]{};
        char buf_icon[256]{};

        unsigned long len_username, len_password, len_email, len_nick, len_desc, len_icon;
        long long out_id = 0;
        int out_sex = 0;

        // 1 id
        bindOutput[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindOutput[0].buffer = &out_id;

        // 2 username
        bindOutput[1].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[1].buffer = buf_username;
        bindOutput[1].buffer_length = sizeof(buf_username);
        bindOutput[1].length = &len_username;

        // 3 password
        bindOutput[2].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[2].buffer = buf_password;
        bindOutput[2].buffer_length = sizeof(buf_password);
        bindOutput[2].length = &len_password;

        // 4 email
        bindOutput[3].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[3].buffer = buf_email;
        bindOutput[3].buffer_length = sizeof(buf_email);
        bindOutput[3].length = &len_email;

        // 5 nick
        bindOutput[4].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[4].buffer = buf_nick;
        bindOutput[4].buffer_length = sizeof(buf_nick);
        bindOutput[4].length = &len_nick;

        // 6 desc
        bindOutput[5].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[5].buffer = buf_desc;
        bindOutput[5].buffer_length = sizeof(buf_desc);
        bindOutput[5].length = &len_desc;

        // 7 icon
        bindOutput[6].buffer_type = MYSQL_TYPE_STRING;
        bindOutput[6].buffer = buf_icon;
        bindOutput[6].buffer_length = sizeof(buf_icon);
        bindOutput[6].length = &len_icon;

        // 8 sex
        bindOutput[7].buffer_type = MYSQL_TYPE_LONG;
        bindOutput[7].buffer = &out_sex;

        mysql_stmt_bind_result(stmt, bindOutput);

        if (mysql_stmt_fetch(stmt) != 0)
        {
            std::cerr << "[GetUserInfoByName] fetch data error: " << mysql_stmt_error(stmt) << std::endl;
            res.reset();
            break;
        }

        // 赋值到结构体
        res->uid = static_cast<int>(out_id);
        res->username.assign(buf_username, len_username);
        res->password.assign(buf_password, len_password);
        res->email.assign(buf_email, len_email);
        res->nick.assign(buf_nick, len_nick);
        res->desc.assign(buf_desc, len_desc);
        res->icon.assign(buf_icon, len_icon);
        res->sex = out_sex;

    } while (false);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    return res;
}

bool MysqlMgr::AddFriendApply(int from_uid, int to_uid)
{
    auto conn = GetConn();
    MYSQL* mysql = conn.get();
    if (!mysql)
    {
        std::cerr << "[AddFriendApply] get mysql connection failed" << std::endl;
        return false;
    }

    // 联合唯一键冲突则空更新，防重复申请
    const std::string sql = R"(
INSERT INTO friend_apply (from_uid, to_uid) VALUES (?, ?)
ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[AddFriendApply] mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        ReturnConn(std::move(conn));
        return false;
    }

    bool ret = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[AddFriendApply] prepare error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 绑定两个int入参
        MYSQL_BIND bindInput[2]{};
        long long val_from = from_uid;
        long long val_to = to_uid;

        bindInput[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[0].buffer = &val_from;

        bindInput[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[1].buffer = &val_to;

        if (mysql_stmt_bind_param(stmt, bindInput) != 0)
        {
            std::cerr << "[AddFriendApply] bind param error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[AddFriendApply] execute error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 插入成功/重复空更新都算成功
        ret = true;
    } while (false);

    // 统一释放资源
    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    ReturnConn(std::move(conn));
    return ret;
}

bool MysqlMgr::GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& apply_list)
{
    apply_list.clear();
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[GetFriendApplyInfo] get mysql connection failed" << std::endl;
        return false;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    // 只查询需要用到的字段，user表主键id匹配from_uid
    const std::string sql = R"(
        SELECT fa.from_uid, u.username, u.desc, u.icon, u.nick, u.sex, fa.status
        FROM friend_apply fa
        LEFT JOIN user u ON fa.from_uid = u.id
        WHERE fa.to_uid = ?
        ORDER BY fa.create_time DESC
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[GetFriendApplyInfo] mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        return false;
    }

    bool ret = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[GetFriendApplyInfo] prepare error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 绑定入参 to_uid
        MYSQL_BIND bind_in{};
        long long val_uid = to_uid;
        bind_in.buffer_type = MYSQL_TYPE_LONGLONG;
        bind_in.buffer = &val_uid;
        mysql_stmt_bind_param(stmt, &bind_in);

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[GetFriendApplyInfo] execute error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        mysql_stmt_store_result(stmt);
        my_ulonglong rows = mysql_stmt_num_rows(stmt);
        if (rows == 0)
        {
            ret = true;
            break;
        }

        // 接收查询的6个字段
        int res_from_uid, res_sex, res_status;
        char buf_name[128]{}, buf_desc[256]{}, buf_icon[256]{}, buf_nick[128]{};
        unsigned long len_name, len_desc, len_icon, len_nick;

        MYSQL_BIND bind_out[7]{};
        // 1 from_uid
        bind_out[0].buffer_type = MYSQL_TYPE_LONG;
        bind_out[0].buffer = &res_from_uid;
        // 2 username
        bind_out[1].buffer_type = MYSQL_TYPE_STRING;
        bind_out[1].buffer = buf_name;
        bind_out[1].buffer_length = sizeof(buf_name);
        bind_out[1].length = &len_name;
        // 3 desc
        bind_out[2].buffer_type = MYSQL_TYPE_STRING;
        bind_out[2].buffer = buf_desc;
        bind_out[2].buffer_length = sizeof(buf_desc);
        bind_out[2].length = &len_desc;
        //4 icon
        bind_out[3].buffer_type = MYSQL_TYPE_STRING;
        bind_out[3].buffer = buf_icon;
        bind_out[3].buffer_length = sizeof(buf_icon);
        bind_out[3].length = &len_icon;
        //5 nick
        bind_out[4].buffer_type = MYSQL_TYPE_STRING;
        bind_out[4].buffer = buf_nick;
        bind_out[4].buffer_length = sizeof(buf_nick);
        bind_out[4].length = &len_nick;
        //6 sex
        bind_out[5].buffer_type = MYSQL_TYPE_LONG;
        bind_out[5].buffer = &res_sex;
        //7 status
        bind_out[6].buffer_type = MYSQL_TYPE_LONG;
        bind_out[6].buffer = &res_status;

        mysql_stmt_bind_result(stmt, bind_out);

        // 循环读取每一条申请记录
        while (mysql_stmt_fetch(stmt) == 0)
        {
            // 必须结构体有无参构造才能这样写
            auto info = std::make_shared<ApplyInfo>();
            info->_uid = res_from_uid;
            info->_name.assign(buf_name, len_name);
            info->_desc.assign(buf_desc, len_desc);
            info->_icon.assign(buf_icon, len_icon);
            info->_nick.assign(buf_nick, len_nick);
            info->_sex = res_sex;
            info->_status = res_status;

            apply_list.push_back(info);
        }

        ret = true;
    } while (false);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    return ret;
}

bool MysqlMgr::AuthFriendApply(int from, int to)
{
    auto conn = GetConn();
    MYSQL* mysql = conn.get();
    if (!mysql)
    {
        std::cerr << "[AuthFriendApply] get mysql connection failed" << std::endl;
        return false;
    }

    // 增加 AND status=0 仅允许处理待审批申请
    const std::string sql = R"(
UPDATE friend_apply SET status = ? WHERE from_uid = ? AND to_uid = ? AND status = ?
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[AuthFriendApply] mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        ReturnConn(std::move(conn));
        return false;
    }

    bool ret = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[AuthFriendApply] prepare error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        MYSQL_BIND bindInput[4]{};
        long long new_status = 1;
        long long apply_user = to;
        long long audit_user = from;
        long long wait_status = 0;

        bindInput[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[0].buffer = &new_status;

        bindInput[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[1].buffer = &apply_user;

        bindInput[2].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[2].buffer = &audit_user;

        bindInput[3].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[3].buffer = &wait_status;

        if (mysql_stmt_bind_param(stmt, bindInput) != 0)
        {
            std::cerr << "[AuthFriendApply] bind param error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[AuthFriendApply] execute error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 判断实际更新行数大于0才算真正同意成功
        my_ulonglong affect = mysql_stmt_affected_rows(stmt);
        if (affect > 0)
        {
            ret = true;
        }
        else
        {
            std::cerr << "[AuthFriendApply] no record updated, already handled or not exist" << std::endl;
        }
    } while (false);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    ReturnConn(std::move(conn));
    return ret;
}

bool MysqlMgr::AddFriend(int me_uid, int peer_uid, const std::string& back_name)
{
    auto conn = GetConn();
    MYSQL* mysql = conn.get();
    if (!mysql)
    {
        std::cerr << "[AddFriend] get mysql connection failed" << std::endl;
        return false;
    }

    // 联合唯一键冲突则不修改，防止重复加好友
    const std::string sql = R"(
INSERT INTO friend (owner_uid, friend_uid, back_name) VALUES (?, ?, ?)
ON DUPLICATE KEY UPDATE owner_uid = owner_uid
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[AddFriend] mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        ReturnConn(std::move(conn));
        return false;
    }

    bool ret = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[AddFriend] prepare error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // ========== 第一条：me_uid 好友列表新增 peer_uid，带备注back_name ==========
        {
            MYSQL_BIND bind[3]{};
            long long owner = me_uid;
            long long friend_id = peer_uid;

            char name_buf[128]{};
            strncpy(name_buf, back_name.c_str(), sizeof(name_buf)-1);
            unsigned long name_len = back_name.size();

            bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[0].buffer = &owner;

            bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[1].buffer = &friend_id;

            bind[2].buffer_type = MYSQL_TYPE_STRING;
            bind[2].buffer = name_buf;
            bind[2].buffer_length = sizeof(name_buf);
            bind[2].length = &name_len;

            if (mysql_stmt_bind_param(stmt, bind) != 0)
            {
                std::cerr << "[AddFriend] bind param 1 error: " << mysql_stmt_error(stmt) << std::endl;
                break;
            }
            if (mysql_stmt_execute(stmt) != 0)
            {
                std::cerr << "[AddFriend] execute 1 error: " << mysql_stmt_error(stmt) << std::endl;
                break;
            }
        }

        // ========== 第二条：peer_uid 好友列表新增 me_uid，对方无备注（空字符串） ==========
        {
            MYSQL_BIND bind[3]{};
            long long owner = peer_uid;
            long long friend_id = me_uid;

            char empty_buf[1]{};
            unsigned long empty_len = 0;

            bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[0].buffer = &owner;

            bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[1].buffer = &friend_id;

            bind[2].buffer_type = MYSQL_TYPE_STRING;
            bind[2].buffer = empty_buf;
            bind[2].buffer_length = sizeof(empty_buf);
            bind[2].length = &empty_len;

            if (mysql_stmt_bind_param(stmt, bind) != 0)
            {
                std::cerr << "[AddFriend] bind param 2 error: " << mysql_stmt_error(stmt) << std::endl;
                break;
            }
            if (mysql_stmt_execute(stmt) != 0)
            {
                std::cerr << "[AddFriend] execute 2 error: " << mysql_stmt_error(stmt) << std::endl;
                break;
            }
        }

        ret = true;
    } while (false);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    ReturnConn(std::move(conn));
    return ret;
}

bool MysqlMgr::GetFriendList(int uid, std::vector<std::shared_ptr<UserInfo>>& friend_list)
{
    friend_list.clear();
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[GetFriendList] get mysql connection failed" << std::endl;
        return false;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    const std::string sql = R"(
        SELECT f.friend_uid, f.back_name, u.username, u.email, u.nick, u.desc, u.icon, u.sex
        FROM friend f
        LEFT JOIN user u ON f.friend_uid = u.id
        WHERE f.owner_uid = ?
        ORDER BY f.add_time DESC
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[GetFriendList] mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        return false;
    }

    bool ret = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[GetFriendList] prepare error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        MYSQL_BIND bind_in{};
        long long val_uid = uid;
        bind_in.buffer_type = MYSQL_TYPE_LONGLONG;
        bind_in.buffer = &val_uid;
        mysql_stmt_bind_param(stmt, &bind_in);

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[GetFriendList] execute error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        mysql_stmt_store_result(stmt);
        my_ulonglong rows = mysql_stmt_num_rows(stmt);
        if (rows == 0)
        {
            ret = true;
            break;
        }

        int res_friend_uid, res_sex;
        char buf_back[128]{}, buf_name[64]{}, buf_email[128]{}, buf_nick[64]{}, buf_desc[512]{}, buf_icon[256]{};
        unsigned long len_back, len_name, len_email, len_nick, len_desc, len_icon;

        MYSQL_BIND bind_out[8]{};
        bind_out[0].buffer_type = MYSQL_TYPE_LONG;
        bind_out[0].buffer = &res_friend_uid;
        bind_out[1].buffer_type = MYSQL_TYPE_STRING;
        bind_out[1].buffer = buf_back;
        bind_out[1].buffer_length = sizeof(buf_back);
        bind_out[1].length = &len_back;
        bind_out[2].buffer_type = MYSQL_TYPE_STRING;
        bind_out[2].buffer = buf_name;
        bind_out[2].buffer_length = sizeof(buf_name);
        bind_out[2].length = &len_name;
        bind_out[3].buffer_type = MYSQL_TYPE_STRING;
        bind_out[3].buffer = buf_email;
        bind_out[3].buffer_length = sizeof(buf_email);
        bind_out[3].length = &len_email;
        bind_out[4].buffer_type = MYSQL_TYPE_STRING;
        bind_out[4].buffer = buf_nick;
        bind_out[4].buffer_length = sizeof(buf_nick);
        bind_out[4].length = &len_nick;
        bind_out[5].buffer_type = MYSQL_TYPE_STRING;
        bind_out[5].buffer = buf_desc;
        bind_out[5].buffer_length = sizeof(buf_desc);
        bind_out[5].length = &len_desc;
        bind_out[6].buffer_type = MYSQL_TYPE_STRING;
        bind_out[6].buffer = buf_icon;
        bind_out[6].buffer_length = sizeof(buf_icon);
        bind_out[6].length = &len_icon;
        bind_out[7].buffer_type = MYSQL_TYPE_LONG;
        bind_out[7].buffer = &res_sex;

        mysql_stmt_bind_result(stmt, bind_out);

        while (mysql_stmt_fetch(stmt) == 0)
        {
            auto user = std::make_shared<UserInfo>();
            user->uid = res_friend_uid;
            user->username.assign(buf_name, len_name);
            user->email.assign(buf_email, len_email);
            user->nick.assign(buf_nick, len_nick);
            user->desc.assign(buf_desc, len_desc);
            user->icon.assign(buf_icon, len_icon);
            user->sex = res_sex;
            user->back.assign(buf_back, len_back);
            friend_list.push_back(user);
        }

        ret = true;
    } while (false);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    ReturnConn(std::move(connPtr));
    return ret;
}

bool MysqlMgr::GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
                                std::vector<std::shared_ptr<ChatThreadInfo>>& threads,
                                bool& loadMore, int& nextLastId)
{
    loadMore = false;
    nextLastId = lastId;
    threads.clear();

    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[GetUserThreads] get mysql connection failed" << std::endl;
        return false;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    const std::string sql = R"(
        WITH all_threads AS (
            SELECT thread_id, 'private' AS type, user1_id, user2_id
            FROM private_chat
            WHERE (user1_id = ? OR user2_id = ?) AND thread_id > ?
            UNION ALL
            SELECT thread_id, 'group' AS type, 0 AS user1_id, 0 AS user2_id
            FROM group_chat_member
            WHERE user_id = ? AND thread_id > ?
        )
        SELECT thread_id, type, user1_id, user2_id
        FROM all_threads
        ORDER BY thread_id
        LIMIT ?;
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[GetUserThreads] mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        return false;
    }

    bool ret = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[GetUserThreads] prepare error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        // 入参依次：userId、userId、lastId、userId、lastId、pageSize+1
        MYSQL_BIND bindInput[6]{};
        long long arg_userId = static_cast<long long>(userId);
        long long arg_lastId = static_cast<long long>(lastId);
        long long arg_limit = static_cast<long long>(pageSize + 1);

        bindInput[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[0].buffer = &arg_userId;

        bindInput[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[1].buffer = &arg_userId;

        bindInput[2].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[2].buffer = &arg_lastId;

        bindInput[3].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[3].buffer = &arg_userId;

        bindInput[4].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[4].buffer = &arg_lastId;

        bindInput[5].buffer_type = MYSQL_TYPE_LONGLONG;
        bindInput[5].buffer = &arg_limit;

        if (mysql_stmt_bind_param(stmt, bindInput) != 0)
        {
            std::cerr << "[GetUserThreads] bind param error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[GetUserThreads] execute error: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        mysql_stmt_store_result(stmt);
        my_ulonglong rows = mysql_stmt_num_rows(stmt);

        // 输出绑定字段顺序 thread_id、type、user1_id、user2_id
        long long out_thread_id, out_user1, out_user2;
        char buf_type[16]{};
        unsigned long len_type;

        MYSQL_BIND bindOut[4]{};
        bindOut[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindOut[0].buffer = &out_thread_id;

        bindOut[1].buffer_type = MYSQL_TYPE_STRING;
        bindOut[1].buffer = buf_type;
        bindOut[1].buffer_length = sizeof(buf_type);
        bindOut[1].length = &len_type;

        bindOut[2].buffer_type = MYSQL_TYPE_LONGLONG;
        bindOut[2].buffer = &out_user1;

        bindOut[3].buffer_type = MYSQL_TYPE_LONGLONG;
        bindOut[3].buffer = &out_user2;

        mysql_stmt_bind_result(stmt, bindOut);

        std::vector<std::shared_ptr<ChatThreadInfo>> tmp;
        while (mysql_stmt_fetch(stmt) == 0)
        {
            auto info = std::make_shared<ChatThreadInfo>();
            info->_thread_id = static_cast<int64_t>(out_thread_id);
            info->_type.assign(buf_type, len_type);
            info->_user1_id = static_cast<int64_t>(out_user1);
            info->_user2_id = static_cast<int64_t>(out_user2);
            tmp.push_back(info);
        }

        // 游标分页探测多出一条代表后面还有数据
        if ((int)tmp.size() > pageSize)
        {
            loadMore = true;
            tmp.pop_back();
        }

        if (!tmp.empty())
        {
            nextLastId = tmp.back()->_thread_id;
        }

        threads = std::move(tmp);
        ret = true;
    } while (false);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    return ret;
}

bool MysqlMgr::CreatePrivateChat(int user1_id, int user2_id, int& thread_id)
{
    thread_id = 0;
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[CreatePrivateChat] get mysql connection failed" << std::endl;
        return false;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    bool ret = false;
    do
    {
        // 关闭自动提交，开启事务
        if (mysql_autocommit(mysql, 0) != 0)
        {
            std::cerr << "[CreatePrivateChat] disable autocommit failed: " << mysql_error(mysql) << std::endl;
            break;
        }

        // 用户id排序，保证存储永远小id放user1_id、大id放user2_id
        long long uid1 = std::min<long long>((long long)user1_id, (long long)user2_id);
        long long uid2 = std::max<long long>((long long)user1_id, (long long)user2_id);

        // 1. 查询私聊会话，加行级排他锁 FOR‑UPDATE
        const std::string checkSql = R"(
            SELECT thread_id FROM private_chat WHERE user1_id = ? AND user2_id = ? FOR UPDATE;
        )";

        MYSQL_STMT* stmtCheck = mysql_stmt_init(mysql);
        if (!stmtCheck)
        {
            std::cerr << "[CreatePrivateChat] stmtCheck init failed: " << mysql_error(mysql) << std::endl;
            mysql_rollback(mysql);
            break;
        }

        if (mysql_stmt_prepare(stmtCheck, checkSql.c_str(), checkSql.size()) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtCheck prepare error: " << mysql_stmt_error(stmtCheck) << std::endl;
            mysql_stmt_close(stmtCheck);
            mysql_rollback(mysql);
            break;
        }

        MYSQL_BIND bindCheck[2]{};
        bindCheck[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindCheck[0].buffer = &uid1;
        bindCheck[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bindCheck[1].buffer = &uid2;
        mysql_stmt_bind_param(stmtCheck, bindCheck);

        if (mysql_stmt_execute(stmtCheck) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtCheck execute error: " << mysql_stmt_error(stmtCheck) << std::endl;
            mysql_stmt_close(stmtCheck);
            mysql_rollback(mysql);
            break;
        }
        mysql_stmt_store_result(stmtCheck);

        long long existThreadId = 0;
        MYSQL_BIND outCheck[1]{};
        outCheck[0].buffer_type = MYSQL_TYPE_LONGLONG;
        outCheck[0].buffer = &existThreadId;
        mysql_stmt_bind_result(stmtCheck, outCheck);

        // 会话已经存在
        if (mysql_stmt_fetch(stmtCheck) == 0)
        {
            thread_id = static_cast<int>(existThreadId);
            mysql_stmt_free_result(stmtCheck);
            mysql_stmt_close(stmtCheck);

            // 提交事务
            mysql_commit(mysql);
            ret = true;
            break;
        }
        mysql_stmt_free_result(stmtCheck);
        mysql_stmt_close(stmtCheck);

        // 2. 插入 chat_thread 会话主表
        const std::string insertThreadSql = "INSERT INTO chat_thread (type, created_at) VALUES ('private', NOW());";
        MYSQL_STMT* stmtInsertThread = mysql_stmt_init(mysql);
        if (!stmtInsertThread)
        {
            std::cerr << "[CreatePrivateChat] stmtInsertThread init failed: " << mysql_error(mysql) << std::endl;
            mysql_rollback(mysql);
            break;
        }
        if (mysql_stmt_prepare(stmtInsertThread, insertThreadSql.c_str(), insertThreadSql.size()) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtInsertThread prepare error: " << mysql_stmt_error(stmtInsertThread) << std::endl;
            mysql_stmt_close(stmtInsertThread);
            mysql_rollback(mysql);
            break;
        }
        if (mysql_stmt_execute(stmtInsertThread) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtInsertThread execute error: " << mysql_stmt_error(stmtInsertThread) << std::endl;
            mysql_stmt_close(stmtInsertThread);
            mysql_rollback(mysql);
            break;
        }
        mysql_stmt_close(stmtInsertThread);

        // 3. 获取自增主键 thread_id
        const std::string lastIdSql = "SELECT LAST_INSERT_ID();";
        MYSQL_STMT* stmtLastId = mysql_stmt_init(mysql);
        if (!stmtLastId)
        {
            std::cerr << "[CreatePrivateChat] stmtLastId init failed: " << mysql_error(mysql) << std::endl;
            mysql_rollback(mysql);
            break;
        }
        if (mysql_stmt_prepare(stmtLastId, lastIdSql.c_str(), lastIdSql.size()) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtLastId prepare error: " << mysql_stmt_error(stmtLastId) << std::endl;
            mysql_stmt_close(stmtLastId);
            mysql_rollback(mysql);
            break;
        }
        if (mysql_stmt_execute(stmtLastId) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtLastId execute error: " << mysql_stmt_error(stmtLastId) << std::endl;
            mysql_stmt_close(stmtLastId);
            mysql_rollback(mysql);
            break;
        }
        mysql_stmt_store_result(stmtLastId);
        long long newThreadId = 0;
        MYSQL_BIND bindLast[1]{};
        bindLast[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindLast[0].buffer = &newThreadId;
        mysql_stmt_bind_result(stmtLastId, bindLast);
        mysql_stmt_fetch(stmtLastId);
        thread_id = static_cast<int>(newThreadId);
        mysql_stmt_free_result(stmtLastId);
        mysql_stmt_close(stmtLastId);

        // 4. 写入 private_chat 私聊关联表
        const std::string insertPrivateSql = R"(
            INSERT INTO private_chat (thread_id, user1_id, user2_id, created_at) VALUES (?, ?, ?, NOW());
        )";
        MYSQL_STMT* stmtInsertPrivate = mysql_stmt_init(mysql);
        if (!stmtInsertPrivate)
        {
            std::cerr << "[CreatePrivateChat] stmtInsertPrivate init failed: " << mysql_error(mysql) << std::endl;
            mysql_rollback(mysql);
            break;
        }
        if (mysql_stmt_prepare(stmtInsertPrivate, insertPrivateSql.c_str(), insertPrivateSql.size()) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtInsertPrivate prepare error: " << mysql_stmt_error(stmtInsertPrivate) << std::endl;
            mysql_stmt_close(stmtInsertPrivate);
            mysql_rollback(mysql);
            break;
        }
        MYSQL_BIND bindPrivate[3]{};
        bindPrivate[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bindPrivate[0].buffer = &newThreadId;
        bindPrivate[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bindPrivate[1].buffer = &uid1;
        bindPrivate[2].buffer_type = MYSQL_TYPE_LONGLONG;
        bindPrivate[2].buffer = &uid2;
        mysql_stmt_bind_param(stmtInsertPrivate, bindPrivate);

        if (mysql_stmt_execute(stmtInsertPrivate) != 0)
        {
            std::cerr << "[CreatePrivateChat] stmtInsertPrivate execute error: " << mysql_stmt_error(stmtInsertPrivate) << std::endl;
            mysql_stmt_close(stmtInsertPrivate);
            mysql_rollback(mysql);
            break;
        }
        mysql_stmt_close(stmtInsertPrivate);

        // 全部操作成功，提交事务
        mysql_commit(mysql);
        ret = true;

    } while (false);

    // 恢复自动提交（好习惯，防止后续复用连接事务状态异常）
    mysql_autocommit(mysql, 1);
    return ret;
}

bool MysqlMgr::AddChatMsg(
    std::vector<std::shared_ptr<ChatMessage>>& chat_datas) {

    if (chat_datas.empty()) {
        return true;
    }

    auto connPtr = GetConn();
    if (!connPtr) {
        std::cerr << "[AddChatMsg] get mysql connection failed" << std::endl;
        return false;
    }

    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    const std::string sql =
        "INSERT INTO chat_message(thread_id, sender_id, recv_id, content, status, type) "
        "VALUES (?, ?, ?, ?, ?, ?)";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        std::cerr << "[AddChatMsg] mysql_stmt_init failed: "
                  << mysql_error(mysql) << std::endl;
        return false;
    }

    bool ret = true;
    mysql_autocommit(mysql, 0);

    do {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
            std::cerr << "[AddChatMsg] prepare failed: "
                      << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        for (auto& chat_data : chat_datas) {
            if (!chat_data) {
                continue;
            }

            long long thread_id = chat_data->thread_id;
            long long sender_id = chat_data->sender_id;
            long long recv_id = chat_data->recv_id;
            long long status = chat_data->status;
            long long type = chat_data->type;

            char* content_buf = chat_data->content.data();
            unsigned long content_len =
                static_cast<unsigned long>(chat_data->content.size());

            MYSQL_BIND bind[6]{};

            bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[0].buffer = &thread_id;

            bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[1].buffer = &sender_id;

            bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[2].buffer = &recv_id;

            bind[3].buffer_type = MYSQL_TYPE_STRING;
            bind[3].buffer = content_buf;
            bind[3].buffer_length = content_len;
            bind[3].length = &content_len;

            bind[4].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[4].buffer = &status;

            bind[5].buffer_type = MYSQL_TYPE_LONGLONG;
            bind[5].buffer = &type;

            if (mysql_stmt_bind_param(stmt, bind) != 0) {
                std::cerr << "[AddChatMsg] bind param failed: "
                          << mysql_stmt_error(stmt) << std::endl;
                ret = false;
                break;
            }

            if (mysql_stmt_execute(stmt) != 0) {
                std::cerr << "[AddChatMsg] execute failed: "
                          << mysql_stmt_error(stmt) << std::endl;
                ret = false;
                break;
            }

            chat_data->message_id =
                static_cast<int>(mysql_insert_id(mysql));
        }

        if (ret == false) {
            break;
        }

        if (mysql_commit(mysql) != 0) {
            std::cerr << "[AddChatMsg] commit failed: "
                      << mysql_error(mysql) << std::endl;
            ret = false;
            break;
        }

        ret = true;
    } while (false);

    if (!ret) {
        mysql_rollback(mysql);
    }

    mysql_autocommit(mysql, 1);
    mysql_stmt_close(stmt);
    return ret;
}

bool MysqlMgr::AddChatMsg(std::shared_ptr<ChatMessage> chat_msg) {
    std::vector<std::shared_ptr<ChatMessage>> chat_datas;
    chat_datas.push_back(std::move(chat_msg));
    return AddChatMsg(chat_datas);
}

bool MysqlMgr::UpdateChatMsgStatus(int message_id, int status) {
    auto connPtr = GetConn();
    if (!connPtr) {
        std::cerr << "[UpdateChatMsgStatus] get mysql connection failed" << std::endl;
        return false;
    }

    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    const std::string sql = "UPDATE chat_message SET status = ? WHERE message_id = ?";
    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt) {
        std::cerr << "[UpdateChatMsgStatus] mysql_stmt_init failed: "
                  << mysql_error(mysql) << std::endl;
        return false;
    }

    bool ret = true;
    long long out_status = static_cast<long long>(status);
    long long out_message_id = static_cast<long long>(message_id);

    do {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0) {
            std::cerr << "[UpdateChatMsgStatus] prepare failed: "
                      << mysql_stmt_error(stmt) << std::endl;
            ret = false;
            break;
        }

        MYSQL_BIND bind[2]{};
        bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[0].buffer = &out_status;
        bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[1].buffer = &out_message_id;

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "[UpdateChatMsgStatus] bind param failed: "
                      << mysql_stmt_error(stmt) << std::endl;
            ret = false;
            break;
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "[UpdateChatMsgStatus] execute failed: "
                      << mysql_stmt_error(stmt) << std::endl;
            ret = false;
            break;
        }
    } while (false);

    mysql_stmt_close(stmt);
    return ret;
}




std::shared_ptr<PageResult> MysqlMgr::LoadChatMsg(int thread_id, int message_id, int page_size)
{
    auto connPtr = GetConn();
    if (!connPtr)
    {
        std::cerr << "[LoadChatMsg] get mysql connection failed" << std::endl;
        return nullptr;
    }
    MysqlConnGuard guard(std::move(connPtr));
    MYSQL* mysql = guard.Raw();

    auto result = std::make_shared<PageResult>();
    result->load_more = false;
    result->next_cursor = message_id;

    const std::string sql = R"(
        SELECT message_id, thread_id, sender_id, recv_id, content, status, created_at, type
        FROM chat_message
        WHERE thread_id = ? AND (? = 0 OR message_id < ?)
        ORDER BY message_id DESC
        LIMIT ?;
    )";

    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt)
    {
        std::cerr << "[LoadChatMsg] mysql_stmt_init failed: " << mysql_error(mysql) << std::endl;
        return nullptr;
    }

    bool ok = false;
    do
    {
        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.size()) != 0)
        {
            std::cerr << "[LoadChatMsg] prepare failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        MYSQL_BIND bind_input[4]{};
        long long arg_thread_id = static_cast<long long>(thread_id);
        long long arg_message_id = static_cast<long long>(message_id);
        long long arg_limit = static_cast<long long>(page_size + 1);

        bind_input[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_input[0].buffer = &arg_thread_id;

        bind_input[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_input[1].buffer = &arg_message_id;

        bind_input[2].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_input[2].buffer = &arg_message_id;

        bind_input[3].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_input[3].buffer = &arg_limit;

        if (mysql_stmt_bind_param(stmt, bind_input) != 0)
        {
            std::cerr << "[LoadChatMsg] bind param failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        if (mysql_stmt_execute(stmt) != 0)
        {
            std::cerr << "[LoadChatMsg] execute failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        mysql_stmt_store_result(stmt);

        long long out_message_id = 0;
        long long out_thread_id = 0;
        long long out_sender_id = 0;
        long long out_recv_id = 0;
        long long out_status = 0;
        long long out_type = 0;
        std::string content_buf(65536, '\0');
        unsigned long len_content = 0;
        char out_chat_time[64]{};
        unsigned long len_chat_time = 0;

        MYSQL_BIND bind_out[8]{};
        bind_out[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_out[0].buffer = &out_message_id;

        bind_out[1].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_out[1].buffer = &out_thread_id;

        bind_out[2].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_out[2].buffer = &out_sender_id;

        bind_out[3].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_out[3].buffer = &out_recv_id;

        bind_out[4].buffer_type = MYSQL_TYPE_STRING;
        bind_out[4].buffer = content_buf.data();
        bind_out[4].buffer_length = static_cast<unsigned long>(content_buf.size());
        bind_out[4].length = &len_content;

        bind_out[5].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_out[5].buffer = &out_status;

        bind_out[6].buffer_type = MYSQL_TYPE_STRING;
        bind_out[6].buffer = out_chat_time;
        bind_out[6].buffer_length = sizeof(out_chat_time);
        bind_out[6].length = &len_chat_time;

        bind_out[7].buffer_type = MYSQL_TYPE_LONGLONG;
        bind_out[7].buffer = &out_type;

        if (mysql_stmt_bind_result(stmt, bind_out) != 0)
        {
            std::cerr << "[LoadChatMsg] bind result failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        std::vector<ChatMessage> tmp;
        int fetch_ret = 0;
        while ((fetch_ret = mysql_stmt_fetch(stmt)) == 0 || fetch_ret == MYSQL_DATA_TRUNCATED)
        {
            ChatMessage msg;
            msg.message_id = static_cast<int>(out_message_id);
            msg.thread_id = static_cast<int>(out_thread_id);
            msg.sender_id = static_cast<int>(out_sender_id);
            msg.recv_id = static_cast<int>(out_recv_id);
            msg.unique_id.clear();
            msg.content.assign(content_buf.data(), len_content);
            msg.chat_time.assign(out_chat_time, len_chat_time);
            msg.status = static_cast<int>(out_status);
            msg.type = static_cast<int>(out_type);
            tmp.push_back(std::move(msg));
        }

        if (fetch_ret != MYSQL_NO_DATA)
        {
            std::cerr << "[LoadChatMsg] fetch failed: " << mysql_stmt_error(stmt) << std::endl;
            break;
        }

        if (static_cast<int>(tmp.size()) > page_size)
        {
            result->load_more = true;
            tmp.pop_back();
        }

        if (!tmp.empty())
        {
            result->next_cursor = tmp.back().message_id;
            std::reverse(tmp.begin(), tmp.end());
        }

        result->messages = std::move(tmp);
        ok = true;
    } while (false);

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);

    if (!ok)
    {
        return nullptr;
    }
    return result;
}
