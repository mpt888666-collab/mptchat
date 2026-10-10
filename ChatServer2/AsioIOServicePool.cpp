//
// Created by mpt on 2026/7/1.
//

#include "AsioIOServicePool.h"
#include <cstddef>
#include <iostream>
#include <memory>

AsioIOServicePool::AsioIOServicePool(std::size_t size) :_works(size), _ioServices(size),  _nextIOService(0){
    for (std::size_t i = 0; i < size; ++i) {
        _works[i] = std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>
        (new boost::asio::executor_work_guard<boost::asio::io_context::executor_type>(_ioServices[i].get_executor()));

        _threads.emplace_back([this, i]() {
            _ioServices[i].run();
        });
    }
}

AsioIOServicePool::~AsioIOServicePool() {
    Stop();
    std::cout << "AsioIOServicePool destruct" << std::endl;
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
    auto &io_service = _ioServices[(_nextIOService++) % _ioServices.size()];
    return io_service;
}

void AsioIOServicePool::Stop() {
    for (auto & work: _works) {
        work.reset();
    }

    for (auto& io_service: _ioServices) {
        io_service.stop();
    }

    for (auto & thread: _threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

