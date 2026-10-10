//
// Created by mpt on 2026/8/24.
//

#include "LogicSystem.h"

#include <utility>
#include "CSession.h"
#include "FileSystem.h"
#include "FileWorker.h"
#include "MysqlMgr.h"
#include "RedisMgr.h"
#include <fstream>
#include "base64.h"
#if defined(_WIN32)
#include <windows.h>
#endif
#include <filesystem>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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


std::string WideToUtf8(const std::wstring& wide) {
#if defined(_WIN32)
    if (wide.empty()) {
        return {};
    }
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                  static_cast<int>(wide.size()), nullptr, 0,
                                  nullptr, nullptr);
    if (len <= 0) {
        return {};
    }
    std::string utf8(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                        static_cast<int>(wide.size()), utf8.data(), len,
                        nullptr, nullptr);
    return utf8;
#else
    // Linux 的 wchar_t 是 4 字节，按 UTF-32 -> UTF-8 手写转换，
    // 不依赖 <windows.h>，与 Windows 下 CP_UTF8 的结果一致
    std::string out;
    out.reserve(wide.size());
    for (wchar_t wc : wide) {
        uint32_t cp = static_cast<uint32_t>(wc);
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
#endif
}

std::string MakeFileOutPath(const boost::filesystem::path& base_path,
                            const std::string& uid_dir,
                            const std::string& file_name) {
#if defined(_WIN32)
    std::filesystem::path out(base_path.wstring());
    out /= ToPath(uid_dir);
    out /= ToPath(file_name);
    return WideToUtf8(out.wstring());
#else
    // Linux 下文件系统路径本身就是 UTF-8 字节串，直接用原生字符串拼。
    // 不能绕 wstring：boost::filesystem::path::wstring() 在 POSIX 下按 locale 转换，
    // 中文路径会按字节展开，再转回 UTF-8 就成了二次编码、路径就错了。
    std::filesystem::path out(base_path.string());
    out /= uid_dir;
    out /= file_name;
    return out.string();
#endif
}

}  // namespace

LogicWork::LogicWork() {
    RegisterCallBacks();

    _worker_thread = std::thread (&LogicWork::DealMsg, this);
}

LogicSystem::LogicSystem() {
    for(int i = 0; i < LOGIC_WORE_THREAD; i++){
        _workerThreads.push_back(std::make_shared<LogicWork>());
    }
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> logic_node, int index) {
    auto work = _workerThreads[index];
    work->postMsgToQue(std::move(logic_node));
}

void LogicWork::postMsgToQue(std::shared_ptr<LogicNode> logic_node) {
    std::unique_lock<std::mutex> lock(_mutex);
    _msg_que.push(std::move(logic_node));
    _cv.notify_one();
}

void LogicWork::DealMsg() {
    for (;;) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this]() {
            return !_msg_que.empty() || _b_stop;
        });

        if (_b_stop) {
            while (!_msg_que.empty()) {
                auto msg_node = _msg_que.front();
                _msg_que.pop();

                lock.unlock();
                std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
                auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
                if (call_back_iter != _fun_callbacks.end()) {
                    try {
                        call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
                            std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
                    } catch (const std::exception& e) {
                        std::cerr << "DealMsg handler exception: " << e.what() << std::endl;
                    }
                } else {
                    std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
                }
                lock.lock();
            }
            return;
        }
        std::vector<std::shared_ptr<LogicNode>> batch_msgs;
        while (!_msg_que.empty()) {
            batch_msgs.push_back(_msg_que.front());
            _msg_que.pop();
        }
        lock.unlock();

        for (auto& msg_node : batch_msgs) {
            std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
            std::string msg_data(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len);
            auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
            if (call_back_iter == _fun_callbacks.end()) {
                std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
                continue;
            }
            try {
                call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
                    std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
            } catch (const std::exception& e) {
                std::cerr << "DealMsg handler exception: " << e.what() << std::endl;
            }
        }
    }
}


void LogicWork::Stop() {
    std::unique_lock<std::mutex> lock(_mutex);
    _b_stop = true;
    _cv.notify_all();
    lock.unlock();
    if (_worker_thread.joinable()) _worker_thread.join();
}

LogicWork::~LogicWork() {
    Stop();
}

void LogicWork::RegisterCallBacks() {
    _fun_callbacks[ID_UPLOAD_HEAD_ICON_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id,const std::string& msg_data) {
        std::cerr << "[UPLOAD_REQ] data=" << msg_data << std::endl;
        json req_json = json::parse(msg_data);
        json root;
        auto md5 = req_json["md5"].get<std::string>();
        auto seq = req_json["seq"].get<int>();
        auto last = req_json["last"].get<int>();
        auto name = req_json["name"].get<std::string>();
        auto total_size = req_json["total_size"].get<int>();
        long long trans_size = req_json["trans_size"].get<long long>();
        auto file_data = req_json["data"].get<std::string>();
        auto uid = req_json["uid"].get<int>();
        auto token = req_json["token"].get<std::string>();
        auto last_seq = req_json["last_seq"].get<int>();

        auto uid_str = std::to_string(uid);

        auto file_path = ConfigMgr::Inst().GetFileOutPath();
        auto file_path_str = MakeFileOutPath(file_path, uid_str, name);

        auto callback = [=](const json &result) {
            json rtvalue = result;
            rtvalue["error"] = ErrorCodes::Success;
            rtvalue["total_size"] = total_size;
            rtvalue["seq"] = seq;
            rtvalue["name"] = name;
            rtvalue["trans_size"] = trans_size;
            rtvalue["last"] = last;
            rtvalue["md5"] = md5;
            rtvalue["uid"] = uid;
            rtvalue["last_seq"] = last_seq;
            session->Send(rtvalue.dump(), ID_UPLOAD_HEAD_ICON_RSP);
        };

        if (seq == 1) {
            std::string token_key = USERTOKENPREFIX + uid_str;
            std::string token_value;
            bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
            if (!success) {
                root["error"] = ErrorCodes::UidInvalid;
                session->Send(root.dump(), ID_UPLOAD_HEAD_ICON_RSP);
                return;
            }

            if (token_value != token) {
                root["error"] = ErrorCodes::TokenInvalid;
                session->Send(root.dump(), ID_UPLOAD_HEAD_ICON_RSP);
                return;
            }
        }
        std::hash<std::string> hash;
        int index = hash(name) % FILE_WORKER_COUNT;
        if (seq == 1) {
            //??????????
            auto file_info = std::make_shared<FileInfo>();
            file_info->_file_path_str = file_path_str;
            file_info->_name = name;
            file_info->_seq = seq;
            file_info->_total_size = total_size;
            file_info->_trans_size = trans_size;
            bool success = RedisMgr::GetInstance()->SetFileInfo(name, file_info);
            auto user_info = MysqlMgr::GetInstance()->GetUserInfoById(uid);
            json redis_root;
            redis_root["uid"] = uid;
            redis_root["pwd"] = user_info->password;
            redis_root["name"] = user_info->username;
            redis_root["email"] = user_info->email;
            redis_root["nick"] = user_info->nick;
            redis_root["desc"] = user_info->desc;
            redis_root["sex"] = user_info->sex;
            redis_root["icon"] = user_info->icon;
            RedisMgr::GetInstance()->Set(USER_BASE_INFO + std::to_string(uid), redis_root.dump());
        }
        else {
            auto file_info = RedisMgr::GetInstance()->GetFileInfo(name);
            if (file_info == nullptr) {
                root["error"] = ErrorCodes::FileNotExists;
                return;
            }
            file_info->_seq = seq;
            file_info->_trans_size = trans_size;
            bool success = RedisMgr::GetInstance()->SetFileInfo(name, file_info);
            if (!success) {
                root["error"] = ErrorCodes::FileSaveRedisFailed;
                session->Send(root.dump(), ID_UPLOAD_HEAD_ICON_RSP);
                return;
            }
        }
        FileSystem::GetInstance()->PostMsgToQue(std::make_shared<FileTask>(session, ID_UPLOAD_HEAD_ICON_REQ, uid, file_path_str, name, seq, total_size,trans_size, last, file_data, callback),index);
    };

    _fun_callbacks[ID_DOWN_LOAD_FILE_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id,const std::string& msg_data) {
        std::cerr << "[DOWNLOAD_REQ] data=" << msg_data << std::endl;
        json req_json = json::parse(msg_data);
        json root;

        auto name = req_json["name"].get<std::string>();
        auto seq = req_json["seq"].get<int>();
        long long trans_size = req_json["trans_size"].get<long long>();
        long long total_size = req_json["total_size"].get<long long>();
        auto token = req_json["token"].get<std::string>();
        auto uid = req_json["uid"].get<int>();
        auto client_path = req_json["client_path"].get<std::string>();

        int owner_uid = uid;
        if (req_json.contains("owner_uid")) {
            owner_uid = req_json["owner_uid"].get<int>();
        }

        std::string token_key = USERTOKENPREFIX + std::to_string(uid);
        std::string token_value;
        bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
        if (!success) {
            root["error"] = ErrorCodes::UidInvalid;
            root["name"] = name;
            session->Send(root.dump(), ID_DOWN_LOAD_FILE_RSP);
            return;
        }

        if (token_value != token) {
            root["error"] = ErrorCodes::TokenInvalid;
            root["name"] = name;
            session->Send(root.dump(), ID_DOWN_LOAD_FILE_RSP);
            return;
        }

        auto file_path = ConfigMgr::Inst().GetFileOutPath();
        auto file_path_str = MakeFileOutPath(file_path, std::to_string(owner_uid), name);

        std::filesystem::path infile_path = ToPath(file_path_str);
        std::ifstream infile;
        infile.open(infile_path.c_str(), std::ios::binary);
        if (!infile) {
            root["error"] = ErrorCodes::FileNotExists;
            root["name"] = name;
            session->Send(root.dump(), ID_DOWN_LOAD_FILE_RSP);
            return;
        }

        infile.seekg(0, std::ios::end);
        std::streamoff file_size = infile.tellg();
        if (file_size < 0) {
            root["error"] = ErrorCodes::FileNotExists;
            root["name"] = name;
            session->Send(root.dump(), ID_DOWN_LOAD_FILE_RSP);
            return;
        }

        constexpr std::streamoff DOWNLOAD_CHUNK_SIZE = 1024;
        std::streamoff offset = static_cast<std::streamoff>(trans_size);
        if (offset < 0) {
            offset = 0;
        }

        if (offset >= file_size) {
            root["error"] = ErrorCodes::Success;
            root["seq"] = seq;
            root["name"] = name;
            root["client_path"] = client_path;
            root["data"] = "";
            root["total_size"] = std::to_string(file_size);
            root["current_size"] = std::to_string(file_size);
            root["is_last"] = true;
            session->Send(root.dump(), ID_DOWN_LOAD_FILE_RSP);
            return;
        }

        std::string chunk(static_cast<size_t>(DOWNLOAD_CHUNK_SIZE), '\0');
        infile.seekg(offset, std::ios::beg);
        infile.read(&chunk[0], DOWNLOAD_CHUNK_SIZE);
        std::streamsize bytes_read = infile.gcount();
        chunk.resize(static_cast<size_t>(bytes_read));

        std::streamoff current_size = offset + bytes_read;

        root["error"] = ErrorCodes::Success;
        root["seq"] = seq;
        root["name"] = name;
        root["client_path"] = client_path;
        root["data"] = base64_encode(chunk);
        root["total_size"] = std::to_string(file_size);
        root["current_size"] = std::to_string(current_size);
        root["is_last"] = (current_size >= file_size);
        session->Send(root.dump(), ID_DOWN_LOAD_FILE_RSP);
    };

    _fun_callbacks[ID_IMG_CHAT_UPLOAD_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id,
        const std::string& msg_data) {
            json req_json = json::parse(msg_data);
            auto md5 = req_json["md5"].get<std::string>();
            auto seq = req_json["seq"].get<int>();
            auto name = req_json["name"].get<std::string>();
            auto total_size = req_json["total_size"].get<int>();
            auto trans_size = req_json["trans_size"].get<int>();
            auto last = req_json["last"].get<int>();
            auto file_data = req_json["data"].get<std::string>();
            auto file_path = ConfigMgr::Inst().GetFileOutPath();
            auto uid = req_json["uid"].get<int>();
            //转化为字符串
            auto uid_str = std::to_string(uid);
            auto file_path_str = MakeFileOutPath(file_path, uid_str, name);
            json rtvalue;

            auto callback = [=](const json& result) {

                // 在异步任务完成后调用
                json rtvalue = result;
                rtvalue["error"] = result.contains("error")
                    ? result["error"].get<int>() : ErrorCodes::Success;
                rtvalue["total_size"] = total_size;
                rtvalue["seq"] = seq;
                rtvalue["name"] = name;
                rtvalue["trans_size"] = trans_size;
                rtvalue["last"] = last;
                rtvalue["md5"] = md5;
                rtvalue["uid"] = uid;
                session->Send(rtvalue.dump(), ID_IMG_CHAT_UPLOAD_RSP);
            };

            // 使用 std::hash 对字符串进行哈希
            std::hash<std::string> hash_fn;
            size_t hash_value = hash_fn(name); // 生成哈希值
            int index = hash_value % FILE_WORKER_COUNT;
            std::cout << "Hash value: " << hash_value << std::endl;

            //第一个包
            if (seq == 1) {
                //构造数据存储
                auto file_info = std::make_shared<FileInfo>();
                file_info->_file_path_str = file_path_str;
                file_info->_name = name;
                file_info->_seq = seq;
                file_info->_total_size = total_size;
                file_info->_trans_size = trans_size;
                bool success = RedisMgr::GetInstance()->SetFileInfo(name, file_info);
                if (!success) {
                    rtvalue["error"] = ErrorCodes::FileSaveRedisFailed;
                    session->Send(rtvalue.dump(), ID_IMG_CHAT_UPLOAD_RSP);
                    return;
                }
            }
            else {
                auto file_info = RedisMgr::GetInstance()->GetFileInfo(name);
                if (file_info == nullptr) {
                    rtvalue["error"] = ErrorCodes::FileNotExists;
                    session->Send(rtvalue.dump(), ID_IMG_CHAT_UPLOAD_RSP);
                    return;
                }
                file_info->_seq = seq;
                file_info->_trans_size = trans_size;
                bool success = RedisMgr::GetInstance()->SetFileInfo(name, file_info);
                if (!success) {
                    rtvalue["error"] = ErrorCodes::FileSaveRedisFailed;
                    session->Send(rtvalue.dump(), ID_IMG_CHAT_UPLOAD_RSP);
                    return;
                }
            }


            FileSystem::GetInstance()->PostMsgToQue(
                std::make_shared<FileTask>(session, ID_IMG_CHAT_UPLOAD_REQ, uid, file_path_str, name, seq, total_size,
                    trans_size, last, file_data, callback),
                index
            );
    };
    _fun_callbacks[ID_FILE_CHAT_UPLOAD_REQ] = [this](std::shared_ptr<CSession> session, const short& msg_id,
        const std::string& msg_data) {
            json req_json = json::parse(msg_data);
            auto md5 = req_json["md5"].get<std::string>();
            auto seq = req_json["seq"].get<int>();
            auto name = req_json["name"].get<std::string>();
            auto total_size = req_json["total_size"].get<int>();
            auto trans_size = req_json["trans_size"].get<int>();
            auto last = req_json["last"].get<int>();
            auto file_data = req_json["data"].get<std::string>();
            auto file_path = ConfigMgr::Inst().GetFileOutPath();
            auto uid = req_json["uid"].get<int>();
            auto uid_str = std::to_string(uid);
            auto file_path_str = MakeFileOutPath(file_path, uid_str, name);
            json rtvalue;

            auto callback = [=](const json& result) {
                json rtvalue = result;
                rtvalue["error"] = result.contains("error")
                    ? result["error"].get<int>() : ErrorCodes::Success;
                rtvalue["total_size"] = total_size;
                rtvalue["seq"] = seq;
                rtvalue["name"] = name;
                rtvalue["trans_size"] = trans_size;
                rtvalue["last"] = last;
                rtvalue["md5"] = md5;
                rtvalue["uid"] = uid;
                session->Send(rtvalue.dump(), ID_FILE_CHAT_UPLOAD_RSP);
            };

            std::hash<std::string> hash_fn;
            size_t hash_value = hash_fn(name);
            int index = hash_value % FILE_WORKER_COUNT;

            if (seq == 1) {
                auto file_info = std::make_shared<FileInfo>();
                file_info->_file_path_str = file_path_str;
                file_info->_name = name;
                file_info->_seq = seq;
                file_info->_total_size = total_size;
                file_info->_trans_size = trans_size;
                bool success = RedisMgr::GetInstance()->SetFileInfo(name, file_info);
                if (!success) {
                    rtvalue["error"] = ErrorCodes::FileSaveRedisFailed;
                    session->Send(rtvalue.dump(), ID_FILE_CHAT_UPLOAD_RSP);
                    return;
                }
            }
            else {
                auto file_info = RedisMgr::GetInstance()->GetFileInfo(name);
                if (file_info == nullptr) {
                    rtvalue["error"] = ErrorCodes::FileNotExists;
                    session->Send(rtvalue.dump(), ID_FILE_CHAT_UPLOAD_RSP);
                    return;
                }
                file_info->_seq = seq;
                file_info->_trans_size = trans_size;
                bool success = RedisMgr::GetInstance()->SetFileInfo(name, file_info);
                if (!success) {
                    rtvalue["error"] = ErrorCodes::FileSaveRedisFailed;
                    session->Send(rtvalue.dump(), ID_FILE_CHAT_UPLOAD_RSP);
                    return;
                }
            }

            FileSystem::GetInstance()->PostMsgToQue(
                std::make_shared<FileTask>(session, ID_FILE_CHAT_UPLOAD_REQ, uid, file_path_str, name, seq, total_size,
                    trans_size, last, file_data, callback),
                index
            );
    };
}

void LogicSystem::AddMD5File(std::string md5, std::shared_ptr<FileInfo> fileinfo) {
    std::lock_guard<std::mutex> lock(_file_mtx);
    _map_md5_files[md5] = fileinfo;
}

std::shared_ptr<FileInfo> LogicSystem::GetFileInfo(std::string md5) {
    std::lock_guard<std::mutex> lock(_file_mtx);
    auto iter = _map_md5_files.find(md5);
    if (iter == _map_md5_files.end()) {
        return nullptr;
    }

    return iter->second;
}





