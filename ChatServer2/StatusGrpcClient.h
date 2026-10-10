//
// Created by mpt on 2026/7/6.
//

#ifndef GETSERVER_STATUSGRPCCLIENT_H
#define GETSERVER_STATUSGRPCCLIENT_H
#include "const.h"
#include "Singleton.h"
#include <queue>
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <grpcpp/channel.h>
#include <condition_variable>
class StatusConPool {
public:
    StatusConPool(const StatusConPool&) = delete;

    StatusConPool& operator=(const StatusConPool&) = delete;

    explicit StatusConPool(std::string host, std::string port,int num = 2);

    ~StatusConPool();

    std::unique_ptr<message::StatusService::Stub> GetConnection();

    void ReturnConnection(std::unique_ptr<message::StatusService::Stub> connection);
private:
    std::mutex _mtx;
    std::condition_variable _cv;
    std::queue<std::unique_ptr<message::StatusService::Stub>> _connections;
    size_t _poolSize;
    std::string _host;
    std::string _port;
    bool _stop{false};

    void Close();
};

class StatusGrpcClient : public Singleton<StatusGrpcClient>{
    friend class Singleton<StatusGrpcClient>;
public:
    message::GetChatServerRsp GetChatServer(int uid);

    message::LoginRsp Login(int uid, std::string token);
private:
    std::unique_ptr<StatusConPool> _pool;

    StatusGrpcClient();
};


#endif //GETSERVER_STATUSGRPCCLIENT_H