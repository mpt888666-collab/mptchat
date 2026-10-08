//
// Created by mpt on 2026/6/30.
//

#ifndef GETSERVER_VARIFYGRPCCLIENT_H
#define GETSERVER_VARIFYGRPCCLIENT_H
#include <queue>
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

class RPConPool {
public:
    RPConPool(const RPConPool&) = delete;
    RPConPool& operator=(const RPConPool&) = delete;

    RPConPool(size_t poolSize, std::string host, std::string port);

    std::unique_ptr<message::VarifyService::Stub> getConnection();

    void returnConnection(std::unique_ptr<message::VarifyService::Stub> context);

    void Close() {
        _b_stop = true;
        _cv.notify_all();
    }

private:
    std::mutex _mtx;
    std::condition_variable _cv;
    size_t _poolSize;
    std::string _host;
    std::string _port;
    std::queue<std::unique_ptr<message::VarifyService::Stub>> _connections;
    bool _b_stop{false};
};

class VarifyGrpcClient : public Singleton<VarifyGrpcClient> {
friend class Singleton<VarifyGrpcClient>;
public:
    message::GetVarifyRsp GetVarifyCode (std::string email) {
        grpc::ClientContext context;
        message::GetVarifyRsp reply;
        message::GetVarifyReq request;

        request.set_email(email);
        auto stub = std::move(_pool->getConnection());
        auto state = stub->GetVarifyCode(&context, request, &reply);
        if (state.ok()) {
            _pool->returnConnection(std::move(stub));
            return reply;
        }else {
            _pool->returnConnection(std::move(stub));
            reply.set_error(ErrorCodes::RPCFailed);
            return reply;
        }
    }
private:
    VarifyGrpcClient() {
        auto& gCfgMgr = ConfigMgr::Inst();
        std::string host = gCfgMgr["VarifyServer"]["Host"];
        std::string port = gCfgMgr["VarifyServer"]["Port"];
        _pool.reset(new RPConPool(5, host, port));
    }

    std::unique_ptr<RPConPool> _pool;
};
#endif //GETSERVER_VARIFYGRPCCLIENT_H