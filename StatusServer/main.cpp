#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include <thread>
#include "RedisMgr.h"
void RunServer() {
    auto cfg = ConfigMgr::Inst();
    std::string server_address(cfg["StatusServer"]["Host"] + ":" + cfg["StatusServer"]["Port"]);

    StatusServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    std::thread connMonitor([&cq]() {
        void* tag;
        bool ok;
        while (cq->Next(&tag, &ok)) {
            if (ok) {
                std::cout << "[StatusServer] 新客户端成功建立grpc连接" << std::endl;
            }
        }
    });
    connMonitor.detach();

    boost::asio::io_context io_context;
    // 创建signal_set用于捕获SIGINT
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);

    // 设置异步等待SIGINT信号
    signals.async_wait([&server](const boost::system::error_code& error, int signal_number) {
        if (!error) {
            std::cout << "Shutting down server..." << std::endl;
            server->Shutdown(); // 优雅地关闭服务器
        }
        });

    // 在单独的线程中运行io_context
    std::thread([&io_context]() { io_context.run(); }).detach();

    server->Wait();
    io_context.stop();
}

int main(int argc, char** argv) {
    try {
        RedisMgr::GetInstance()->Connect();
        RunServer();
    }catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;
}