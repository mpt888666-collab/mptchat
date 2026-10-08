//
// Created by mpt on 2026/8/24.
//

#include "FileWorker.h"

#include <fstream>
#include <filesystem>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#endif

#include "ConfigMgr.h"
#include "base64.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"

namespace {

#if defined(_WIN32)
std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return {};
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                  static_cast<int>(utf8.size()), nullptr, 0);
    if (len <= 0) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                        static_cast<int>(utf8.size()), wide.data(), len);
    return wide;
}
#endif

// 跨平台工具：把 UTF-8 字符串转成文件系统路径
// Windows 需要转宽字符才能正确处理中文路径；Linux 直接用原字符串
std::filesystem::path ToPath(const std::string& utf8) {
#if defined(_WIN32)
    return std::filesystem::path(Utf8ToWide(utf8));
#else
    return std::filesystem::path(utf8);
#endif
}


bool OpenUtf8File(std::ofstream& outfile,
                  const std::string& utf8_path,
                  bool truncate) {
    std::filesystem::path file_path = ToPath(utf8_path);
    std::filesystem::path dir_path = file_path.parent_path();
    std::error_code ec;
    if (!dir_path.empty() && !std::filesystem::exists(dir_path, ec)) {
        std::filesystem::create_directories(dir_path, ec);
    }
    outfile.open(file_path.c_str(),
                 std::ios::binary | (truncate ? std::ios::trunc : std::ios::app));
    return static_cast<bool>(outfile);
}

}  // namespace

class LogicNode;

FileWorker::FileWorker():_b_stop(false)
{
    RegisterHandlers();

    _work_thread = std::thread([this]() {
        for (;;) {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this]() {
                return _b_stop || !_task_que.empty();
            });

            if (_b_stop) {
                while (!_task_que.empty()) {
                    auto task = _task_que.front();
                    _task_que.pop();
                    task_callback(task);
                }
                break;
            }

            if (_task_que.empty()) {
                continue;
            }

            auto task = _task_que.front();
            _task_que.pop();
            lock.unlock();
            task_callback(task);
        }

    });
}


FileWorker::~FileWorker()
{
    _b_stop = true;
    _cv.notify_one();
    _work_thread.join();
}

void FileWorker::PostTask(std::shared_ptr<FileTask> task)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _task_que.push(task);
    }

    _cv.notify_one();
}

void FileWorker::task_callback(std::shared_ptr<FileTask> task)
{
    if (!task) {
        return;
    }

    auto iter = _handlers.find(task->_msg_id);
    if (iter == _handlers.end()) {
        return;
    }

    iter->second(task);
}

void FileWorker::RegisterHandlers()
{
    //处理头像上传
    _handlers[ID_UPLOAD_HEAD_ICON_REQ] = [this](std::shared_ptr<FileTask> task) {
        std::string decoded = base64_decode(task->_file_data);

    auto file_path_str = task->_file_path_str;
    auto last = task->_last;

    std::filesystem::path file_path = ToPath(file_path_str);
    std::string filename = file_path.filename().string();
    json result;

    std::ofstream outfile;
    if (!OpenUtf8File(outfile, file_path_str, task->_seq == 1)) {
        std::cerr << "Failed to open file for writing: " << file_path_str << std::endl;
        result["error"] = ErrorCodes::FileWritePermissionFailed;
        task->_callback(result);
        return;
    }

    if (!outfile) {
        std::cerr << "Failed to open file for writing: " << file_path_str << std::endl;
        result["error"] = ErrorCodes::FileWritePermissionFailed;
        task->_callback(result);
        return ;
    }

    outfile.write(decoded.data(), decoded.size());
    if (!outfile) {
        std::cerr << "Failed to write file: " << file_path_str << std::endl;
        result["error"] = ErrorCodes::FileWritePermissionFailed;
        task->_callback(result);
        return ;
    }

    outfile.close();
    if (last) {
        std::cout << "File saved successfully: " << task->_name << std::endl;
        bool icon_ok = MysqlMgr::GetInstance()->UpdateUserIcon(task->_uid, filename);
        std::cerr << "[FileWorker] UpdateUserIcon uid=" << task->_uid
                  << " name=" << filename << " ret=" << (icon_ok ? 1 : 0) << std::endl;

        auto user_info = MysqlMgr::GetInstance()->GetUser(task->_uid);
        if (user_info == nullptr) {
            return ;
        }

        json redis_root;
        redis_root["uid"] = task->_uid;
        redis_root["pwd"] = user_info->password;
        redis_root["name"] = user_info->username;
        redis_root["email"] = user_info->email;
        redis_root["nick"] = user_info->nick;
        redis_root["desc"] = user_info->desc;
        redis_root["sex"] = user_info->sex;
        redis_root["icon"] = user_info->icon;
        std::string base_key = USER_BASE_INFO + std::to_string(task->_uid);
        RedisMgr::GetInstance()->Set(base_key, redis_root.dump());
        //return;
    }

    if (task->_callback) {
        task->_callback(result);
    }
    };

    _handlers[ID_IMG_CHAT_UPLOAD_REQ] = [this](std::shared_ptr<FileTask> task) {
        std::string decoded = base64_decode(task->_file_data);

        auto file_path_str = task->_file_path_str;
        auto last = task->_last;

        json result;

        std::ofstream outfile;
        if (!OpenUtf8File(outfile, file_path_str, task->_seq == 1)) {
            std::cerr << "Failed to open file for writing: " << file_path_str << std::endl;
            result["error"] = ErrorCodes::FileWritePermissionFailed;
            task->_callback(result);
            return;
        }

        outfile.write(decoded.data(), decoded.size());
        if (!outfile) {
            std::cerr << "Failed to write file: " << file_path_str << std::endl;
            result["error"] = ErrorCodes::FileWritePermissionFailed;
            task->_callback(result);
            return;
        }

        outfile.close();
        if (task->_callback) {
            task->_callback(result);
        }
    };
    _handlers[ID_FILE_CHAT_UPLOAD_REQ] = [this](std::shared_ptr<FileTask> task) {
        std::string decoded = base64_decode(task->_file_data);

        auto file_path_str = task->_file_path_str;
        auto last = task->_last;

        json result;

        std::ofstream outfile;
        if (!OpenUtf8File(outfile, file_path_str, task->_seq == 1)) {
            std::cerr << "Failed to open file for writing: " << file_path_str << std::endl;
            result["error"] = ErrorCodes::FileWritePermissionFailed;
            task->_callback(result);
            return;
        }

        outfile.write(decoded.data(), decoded.size());
        if (!outfile) {
            std::cerr << "Failed to write file: " << file_path_str << std::endl;
            result["error"] = ErrorCodes::FileWritePermissionFailed;
            task->_callback(result);
            return;
        }

        outfile.close();
        if (task->_callback) {
            task->_callback(result);
        }
    };
}
