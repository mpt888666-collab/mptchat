//
// Created by mpt on 2026/6/28.
//

#ifndef MPTCHAT_SINGLETON_H
#define MPTCHAT_SINGLETON_H
#include <memory>
#include <mutex>
#include <iostream>
template <typename T>
class Singleton {
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;

    static std::shared_ptr<T> _instance;
public:
    static std::shared_ptr<T> instance() {
        static std::once_flag once;
        std::call_once(once, []() {
            _instance = std::shared_ptr<T>(new T);
        });
        return _instance;
    }

    void PrintAddress() {
        std::cout << _instance.get() << std::endl;
    }

    virtual ~Singleton() {
        std::cout << "this is singleton destruct" << std::endl;
    }
};


template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
#endif //MPTCHAT_SINGLETON_H