//
// Created by mpt on 2026/8/24.
//

#ifndef RESOURCESERVER_LOGICSYSTEM_H
#define RESOURCESERVER_LOGICSYSTEM_H
#include "Singleton.h"
#include <vector>
#include <queue>
#include <map>
#include <functional>

#include "const.h"
class LogicNode;
class LogicSystem;
class CSession;
class FileInfo {
public:
    FileInfo(int seq = 0, std::string name = "", int total_size = 0,
        int trans_size = 0, std::string file_path_str = "")
        :_seq(seq), _name(name), _total_size(total_size),
        _trans_size(trans_size), _file_path_str(file_path_str){}
    int _seq;
    std::string _name;
    int _total_size;
    int _trans_size;
    std::string _file_path_str;
};
class LogicWork {
public:
    LogicWork();
    void postMsgToQue(std::shared_ptr<LogicNode>);
    void Stop();
    ~LogicWork();
private:
    void RegisterCallBacks();
    void DealMsg();
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _cv;
    bool _b_stop{};
    std::thread _worker_thread;
    std::map<uint16_t, std::function<void(std::shared_ptr<CSession>, uint16_t msg_id, const std::string &msg_data)>> _fun_callbacks;
};


class LogicSystem : public Singleton<LogicSystem>{
    friend Singleton<LogicSystem>;
    friend LogicWork;
public:
    void PostMsgToQue(std::shared_ptr<LogicNode>, int index);
    void AddMD5File(std::string md5, std::shared_ptr<FileInfo> fileinfo);
    std::shared_ptr<FileInfo> GetFileInfo(std::string md5);
private:
    LogicSystem();
    std::vector<std::shared_ptr<LogicWork>> _workerThreads;
    std::mutex _file_mtx;
    std::unordered_map<std::string, std::shared_ptr<FileInfo>> _map_md5_files;
};


#endif //RESOURCESERVER_LOGICSYSTEM_H