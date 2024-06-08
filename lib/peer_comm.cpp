#include "peer_comm.h"
#include <map>

json key_sync::KEY_SYNC(const URI &source, const URI &destination, const Status status, IndexBuffer &indexes,const QoS &qos){
    json key_sync_json ={
        {"command", "KEY_SYNC"},
        {"data", {
            {"source", source},
            {"destination", destination},
            {"qos", {
                {"key_type", qos.key_type},
                {"key_chunk_size", qos.key_chunk_size},
            }
        },
        {"status", status},
        {"indexBuffer", indexes},
        },
        },
    };
    return key_sync_json;
}

json key_sync::NEW_KEY(const URI &source, const URI &destination, const UUID &key_stream_id, const unsigned int &id, const QoS &qos, const std::map<int,std::vector<int>> &indexes){
    json new_key_json={
        {"command", "NEW_KEY"},
        {"data", {
            {"source", source},
            {"destination", destination},
            {"key_stream_id", key_stream_id},
            {"id", id},
            {"qos", {
                    {"key_type", qos.key_type},
                    {"key_chunk_size", qos.key_chunk_size},
                }
            },
            {"indexes", indexes}
        },
        },
    };
    return new_key_json;
}

json key_sync::NEW_KEY_ACK(const URI &source, const URI &destination, const UUID &key_stream_id, const unsigned int &id){
    json new_key_ack_json={
        {"command", "NEW_KEY_ACK"},
        {"data", {
            {"source", source},
            {"destination", destination},
            {"key_stream_id", key_stream_id},
            {"id", id},
        },
        },
    };
    return new_key_ack_json;
}

// INTERNAL COMM
json key_sync::SYNC_INDEX(const std::vector<unsigned int> &sync_indexes){
    json sync_index_json={
        {"indexes", sync_indexes},
    };
    return sync_index_json;
}

json key_sync::DISCARD(const std::vector<unsigned int> &sync_indexes){
    json sync_index_json={
        {"indexes", sync_indexes},
    };
    return sync_index_json;
}

json key_sync::SESSION(const UUID &key_stream_id,const unsigned int &msg_index, const unsigned int &keyType, const unsigned int &key_chunk_size){
    json session_json={
        {"key_stream_id", key_stream_id},
        {"msg_index", msg_index},
        {"key_type", keyType},
        {"key_chunk_size", key_chunk_size},
    };
    return session_json;
}
