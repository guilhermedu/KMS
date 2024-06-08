#ifndef PEER_COMM_H
#define PEER_COMM_H

#include <unistd.h>
#include <uuid/uuid.h>

#include <string>
#include <stdexcept>
#include "json.hpp"
#include <thread>
#include "netxpto_20200819.h"

using json = nlohmann::ordered_json;

namespace key_sync {

    using URI = std::string;
    using UUID = std::string;
    using IndexBuffer = typename std::vector<t_string>;

    const unsigned int SYMMETRIC = 0;
    const unsigned int OBLIVIOUS = 1;

    struct QoS {
        unsigned int key_type;
        unsigned int key_chunk_size;
    };

    enum Status : unsigned int {
        SUCCESSFUL = 0, PEER_NOT_CONNECTED = 1, GK_INSUFFICIENT_KEY_AVAILABLE = 2, GK_PEER_NOT_CONNECTED = 3,
        NO_QKD_CONNECTION_AVAILABLE = 4, OC_KSID_ALREADY_IN_USE = 5, TIMEOUT_ERROR = 6, OC_QOS_NOT_SATISFIABLE = 7,
        GK_METADATA_INSUFFICIENT_SIZE = 8
    };

    // json with message to notify peer of what key material it was received from the physical layer.
    json KEY_SYNC(const URI &source, const URI &destination, const Status status, IndexBuffer &indexes,const QoS &qos);

    // json with message to be sent to the peer kms to establish the creation of a key to be provided to the app on and by both kms
    json NEW_KEY(const URI &source, const URI &destination, const UUID &key_stream_id, const unsigned int &id, const QoS &qos, const std::map<int,std::vector<int>> &indexes);

    json NEW_KEY_ACK(const URI &source, const URI &destination, const UUID &key_stream_id, const unsigned int &id);

    json SYNC_INDEX(const std::vector<unsigned int> &sync_indexes);    
    
    json DISCARD(const std::vector<unsigned int> &sync_indexes);

    json SESSION(const UUID &key_stream_id, const unsigned int &msg_index, const unsigned int &keyType, const unsigned int &key_chunk_size);

}

#endif