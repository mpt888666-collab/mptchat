//
// Created by mpt on 2026/8/23.
//

#ifndef RESOURCESERVER_MSGNODE_H
#define RESOURCESERVER_MSGNODE_H
#include <cstdint>
#include <cstring>
#include <iostream>
class CSession;
class LogicWork;
class MsgNode {
public:
    MsgNode(const MsgNode&) = delete;
    MsgNode& operator=(const MsgNode&) = delete;
    explicit MsgNode(uint32_t len) : _cur_len(0), _total_len(len){
        _data = new char[_total_len + 1]();
        _data[_total_len] = '\0';
    }
    ~MsgNode() {
        std::cout << "destruct MsgNode" << std::endl;
        delete[] _data;
    }

    void Clear() {
        memset(_data, '\0', _total_len);
    }

    char * _data;
    uint16_t _cur_len;
    uint16_t _total_len;
};

class RecvNode : public MsgNode {
    friend class LogicSystem;
    friend LogicWork;
public:
    RecvNode(uint16_t id, uint32_t len);
private:
    uint16_t _msg_id;
};

class SendNode : public MsgNode {
    friend class LogicSystem;
public:
    SendNode(const char * msg, uint16_t max_len, uint16_t msg_id);
private:
    uint16_t _msg_id;
};


#endif //RESOURCESERVER_MSGNODE_H