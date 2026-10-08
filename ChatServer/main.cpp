#include <iostream>
#include "AsioIOServicePool.h"
#include "ConfigMgr.h"
#include <boost/asio.hpp>
#include "CServer.h"
#include "RedisMgr.h"
#include "const.h"
#include "message.grpc.pb.h"
#include "ChatServiceImpl.h"
#include "LogicSystem.h"

int main() {
    try {

        RedisMgr::GetInstance()->Connect();
        auto& cfg = ConfigMgr::Inst();

        auto server_name = cfg["SelfServer"]["Name"];
        RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
        std::string server_address = cfg["SelfServer"]["Host"] + ':' + cfg["SelfServer"]["RPCPort"];
        ChatServiceImpl service;
        grpc::ServerBuilder builder;
        builder.RegisterService(&service);
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

        std::shared_ptr<grpc::Server> server(builder.BuildAndStart());
        std::thread  grpc_server_thread([&server]() {
                server->Wait();
            });


        auto pool = AsioIOServicePool::GetInstance();
        auto port = cfg["SelfServer"]["Port"];
        auto host = cfg["SelfServer"]["Host"];

        boost::asio::io_context ioc;
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

        signals.async_wait([&ioc, pool](auto, auto) {
            ioc.stop();
            pool->Stop();
        });
        auto pointer_server = std::make_shared<CServer>(ioc, atoi(port.c_str()));
        service.RegisterServer(pointer_server);

        // Must set server BEFORE ioc.run() — login handler uses _p_server
        LogicSystem::GetInstance()->SetServer(pointer_server);

        ioc.run();

        RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
        RedisMgr::GetInstance()->Close();
        grpc_server_thread.join();
    }catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
