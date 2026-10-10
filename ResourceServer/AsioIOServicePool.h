//
// Created by mpt on 2026/7/1.
//
#ifndef GETSERVER_ASIOIOSERVICEPOOL_H
#define GETSERVER_ASIOIOSERVICEPOOL_H
#include "const.h"
#include <thread>

class AsioIOServicePool : public Singleton<AsioIOServicePool> {
    friend class Singleton<AsioIOServicePool>;
public:
    AsioIOServicePool(AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(AsioIOServicePool&) = delete;
    ~AsioIOServicePool();

    boost::asio::io_context& GetIOService();

    void Stop();

private:
    explicit AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());

    std::vector<std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>> _works;
    std::vector<boost::asio::io_context> _ioServices;
    std::vector<std::thread> _threads;
    std::size_t _nextIOService;
};


#endif //GETSERVER_ASIOIOSERVICEPOOL_H