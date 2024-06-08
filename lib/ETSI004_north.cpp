#include "ETSI004_north.h"
#include "etsi_qkd_004.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// in(0-request, 1-generated_key)
// out(0-response, 1-SessionInfo)

void ETSI004North::initialize(void){
      
}

bool ETSI004North::runBlock(void){

    bool alive = true;
    if (getTerminated()) return false;

    std::cout << "[ETSI_NORTH]: ENTER" << std::endl;
    
    auto ready = inputSignals[0]->ready();
    auto readyKey = inputSignals[1]->ready();

    if (waitingKey && readyKey){
        etsi_qkd_004::KeyBuffer keyBuffer;
        for(auto k = 0; k < Sessions[KSID].key_chunk_size ; k++){
            t_binary kval{0};
            inputSignals[1]->bufferGet(&kval);
            keyBuffer.push_back(kval);
        }
        t_message msgSend;
        t_string msgDataSend = etsi_qkd_004::handle_get_key(status,keyBuffer,Sessions[key_stream_id].msg_index++,metadata_server).dump();
        msgSend.setMessageData(msgDataSend);
        outputSignals[0]->bufferPut(msgSend);
        if(getVerboseMode()){
            std::cout << ("[004-NORTH]: ") << "SENT GET_KEY_RESPONSE" << std::endl;
        }
        waitingKey = false;
    }

    if (ready){
        t_message msgReceived;
        inputSignals[0]->bufferGet(&msgReceived);
        receivedMessages.push_back(msgReceived);
        msgJson = json::parse(msgReceived.getMessageData());
        msgCommand = msgJson["command"];
        msgData = msgJson["data"];

        if (msgCommand == "OPEN_CONNECT"){
            if(getVerboseMode()){
                std::cout << ("[004-NORTH]: ") << "RECEIVED OPEN_CONNECT" << std::endl;
            }
            
            // Check key_stream_id
            if (!msgData.contains("key_stream_id")) {

                // generate random UUID
                boost::uuids::random_generator generator;
                boost::uuids::uuid uuid = generator();
                KSID = boost::uuids::to_string(uuid);

                // verify if already exists and generate another UUID if so
                while(Sessions.find(KSID) != Sessions.end()){
                    uuid = generator();
                    KSID = boost::uuids::to_string(uuid);
                }

                SessionInfo info;
                info.msg_index = 0;
                info.status = "OPEN";
                Sessions[KSID] = info;
            } else {
                KSID = msgData["key_stream_id"];
                if (Sessions.find(KSID) == Sessions.end()) {
                    SessionInfo info;
                    info.msg_index = 0;
                    info.status = "OPEN";
                    Sessions[KSID] = info;
                } else { status = etsi_qkd_004::OC_KSID_ALREADY_IN_USE;}
            }

            // save key_type and key_chunk_size into session
            unsigned int key_type = msgData["qos"]["key_type"];
            unsigned int key_chunk_size = msgData["qos"]["key_chunk_size"];

            Sessions[KSID].key_type = key_type;
            Sessions[KSID].key_chunk_size = key_chunk_size;

            unsigned int max_bps = msgData["qos"]["max_bps"];
            unsigned int min_bps = msgData["qos"]["min_bps"];
            unsigned int jitter = msgData["qos"]["jitter"];
            unsigned int priority = msgData["qos"]["priority"];
            unsigned int timeout = msgData["qos"]["timeout"];
            unsigned int ttl = msgData["qos"]["ttl"];
            std::string metadata_mimetype = msgData["qos"]["metadata_mimetype"];
            setQoS(key_type, key_chunk_size, max_bps, min_bps, jitter, priority, timeout, ttl, metadata_mimetype);
            
            // send response
            t_message msgSend;
            t_string msgDataSend = etsi_qkd_004::handle_open_connect(KSID,getQoS(),status).dump();
            msgSend.setMessageData(msgDataSend);
            outputSignals[0]->bufferPut(msgSend);
        } else if (msgCommand == "GET_KEY"){
            if(getVerboseMode()){
                std::cout << ("[004-NORTH]: ") << "RECEIVED GET_KEY" << std::endl;
            }
            // obtain KSID and send Session Info to load_db
            KSID = msgData["key_stream_id"];
            Sessions[KSID].msg_index = msgData["index"];
            
            t_message sendSession;
            t_string sendSessionData = key_sync::SESSION(KSID,Sessions[KSID].msg_index,Sessions[KSID].key_type, Sessions[KSID].key_chunk_size).dump();
            sendSession.setMessageData(sendSessionData);
            outputSignals[1]->bufferPut(sendSession);
            waitingKey = true;

        } else if (msgCommand == "CLOSE"){
            if(getVerboseMode()){
                std::cout << ("[004-NORTH]: ") << "RECEIVED CLOSE" << std::endl;
            }

            KSID = msgData["key_stream_id"];
            Sessions[KSID].status = "CLOSE";

            // send response
            t_message msgSend;
            t_string msgDataSend = etsi_qkd_004::handle_close(status).dump();
            msgSend.setMessageData(msgDataSend);
            outputSignals[0]->bufferPut(msgSend);

            if(getVerboseMode()){
                for(const auto& msg : receivedMessages){
                    std::cout << "MSG: " << i << " - " << msg << std::endl;
                    i++;
                } 
            }
        }
    } 
    std::cout << "[ETSI_NORTH]: EXIT" << std::endl;
    return true;
}