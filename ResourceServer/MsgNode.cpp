//
// Created by mpt on 2026/8/23.
//

#include "MsgNode.h"
#include "const.h"
#include <boost/asio.hpp>
RecvNode::RecvNode(uint16_t id, uint32_t len) : MsgNode(len), _msg_id(id){}

SendNode::SendNode(const char *msg, uint16_t len, uint16_t id) : MsgNode(len + HEAD_TOTAL_LEN), _msg_id(id){
    auto net_id = boost::asio::detail::socket_ops::host_to_network_short(_msg_id);
    auto net_len = boost::asio::detail::socket_ops::host_to_network_long(len);

    memcpy(_data, &net_id, HEAD_ID_LEN);
    memcpy(_data + HEAD_ID_LEN, &net_len, HEAD_DATA_LEN);
    memcpy(_data + HEAD_TOTAL_LEN, msg, len);
}

