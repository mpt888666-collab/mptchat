//
// Created by mpt on 2026/7/3.
//

#include "MysqlMgr.h"
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
            outInfo->name.assign(buf_name, len_name);
            outInfo->email.assign(buf_email, len_email);
            outInfo->pwd.assign(buf_pwd, len_pwd);
        }
    } while (0);

    mysql_stmt_close(stmt);
    return succ;
}
