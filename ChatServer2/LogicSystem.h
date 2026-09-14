//
// Created by mpt on 2026/7/8.
//

#ifndef CHATSERVER_LOGICSYSTEM_H
#define CHATSERVER_LOGICSYSTEM_H
#include "const.h"
class CServer;
class LogicNode;
class CSession;
struct UserInfo;
typedef  std::function<void(std::shared_ptr<CSession>, uint16_t msg_id, const std::string &msg_data)> FunCallBack;
class LogicSystem : public Singleton<LogicSystem>{
    friend class Singleton<LogicSystem>;

public:
    ~LogicSystem() {
        _b_stop = true;
        _consume.notify_one();
        _worker_thread.join();
    }

    void PostMsgToQue(std::shared_ptr<LogicNode> msg);

    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo> user_info);

    void SetServer(std::shared_ptr<CServer> pserver);

private:
    std::queue<std::shared_ptr<LogicNode>> _msg_que;

    std::thread _worker_thread;

    std::mutex _mutex;

    std::condition_variable _consume;

    std::map<uint16_t, FunCallBack> _fun_callbacks;

    std::shared_ptr<CServer> _p_server;

    bool _b_stop;

    void RegisterCallBacks();

    LogicSystem();

    void DealMsg();

    bool isPureDigit(std::string);

    void GetUserByUid(std::string, json&);

    void GetUserByName(std::string, json&);

    void NotifyFileChatMsg(int from_uid, int to_uid, int thread_id, int message_id,
        const std::string& unique_id, const std::string& unique_name,
        const std::string& md5, const std::string& chat_time, int status, int type, int total_size);
};


#endif //CHATSERVER_LOGICSYSTEM_H
