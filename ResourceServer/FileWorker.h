//
// Created by mpt on 2026/8/24.
//

#ifndef RESOURCESERVER_FILEWORKER_H
#define RESOURCESERVER_FILEWORKER_H

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <map>

#include "const.h"
#include <atomic>
class CSession;
struct FileTask {
    FileTask(std::shared_ptr<CSession> session, MSG_IDS msg_id, int uid, std::string file_path_str, std::string name,
        int seq, int total_size, int trans_size, int last,
        std::string file_data, std::function<void(const json &result)> callback = nullptr) :_session(session), _msg_id(msg_id), _uid(uid), _file_path_str(file_path_str),
        _seq(seq),_name(name),_total_size(total_size),
        _trans_size(trans_size),_last(last),_file_data(file_data), _callback(callback)
    {}
    ~FileTask(){}
    std::shared_ptr<CSession> _session;
    MSG_IDS _msg_id;
    int _uid;
    int _seq ;
    std::string _file_path_str;
    std::string _name ;
    int _total_size ;
    int _trans_size ;
    int _last ;
    std::string _file_data;
    std::function<void(const json &result)> _callback;
};

class FileWorker
{
public:
    FileWorker();

    ~FileWorker();

    void task_callback(std::shared_ptr<FileTask> task);

    void RegisterHandlers();

    void PostTask(std::shared_ptr<FileTask> task);
private:
    std::thread _work_thread;
    std::queue<std::shared_ptr<FileTask>> _task_que;
    std::atomic<bool> _b_stop;
    std::mutex  _mutex;
    std::condition_variable _cv;

    std::unordered_map<MSG_IDS, std::function<void(std::shared_ptr<FileTask>)>> _handlers;
};



#endif //RESOURCESERVER_FILEWORKER_H