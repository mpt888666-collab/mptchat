#include <iostream>
#include <boost/asio.hpp>

#include "AsioIOServicePool.h"
#include "ConfigMgr.h"
#include "CServer.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
int main() {
    auto &cfg = ConfigMgr::Inst();

    if (!RedisMgr::GetInstance()->Connect()) {
        std::cerr << "Redis connect failed" << std::endl;
        return 1;
    }

    auto mysql = MysqlMgr::GetInstance();
    if (!mysql->IsConnected()) {
        std::cerr << "[main] MySQL init failed" << std::endl;
    } else {
        std::cout << "[main] MySQL connected" << std::endl;
    }

    auto host = cfg["SelfServer"]["Host"];
    auto port = cfg["SelfServer"]["Port"];
    boost::asio::io_context ioc;
    auto pool = AsioIOServicePool::GetInstance();
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait([&ioc, pool](auto, auto) {
        ioc.stop();
        pool->Stop();
    });
    auto pointer_server = std::make_shared<CServer>(ioc, atoi(port.c_str()));
    ioc.run();
    return 0;
}