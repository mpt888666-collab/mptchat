#include <iostream>
#include <string>
// 必须正确包含头文件
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include "CServer.h"
#include "const.h"
#include "RedisMgr.h"
#include <exception>
#include <memory>
int main()
{
    MysqlConnGuard conn_guard(MysqlMgr::GetInstance()->GetConn());
    try
    {
        if (!RedisMgr::GetInstance()->Connect())
        {
            std::cerr << "Redis连接失败！" << std::endl;
            return -1;
        }
        std::string gate_port_str = ConfigMgr::Inst()["GateServer"]["Port"];
        unsigned short port = atoi(gate_port_str.c_str());
        boost::asio::io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            ioc.stop();
            });
        std::shared_ptr<CServer> server = std::make_shared<CServer>(ioc, port);
        server->start();
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}