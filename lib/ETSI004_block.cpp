#include "ETSI004_block.h"
#include "etsi_qkd_004.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// pull mode não está a ter em conta se recebe verdadeiramente um novo get_key


void ETSI004Block::initialize(void){
      
}

bool ETSI004Block::runBlock(void){

    bool alive = true;
    if (getTerminated()) return false;

    std::cout << "[ETSI_SOUTH]: ENTER" << std::endl;

    if (getFirstTime()){
        // get_keyResID = 0; // need to be reseted if multiple KSID
        // get_keyID = 0;   // need to be reseted if multiple KSID
        
        if (ID == "Tx"){
            //fazer pedido open_connect
            t_string msgDataSend = etsi_qkd_004::open_connect(source,destination,qos,key_stream_id).dump();
            t_message msgSend;
            msgSend.setMessageData(msgDataSend);
            outputSignals[0]->bufferPut(msgSend);
        }
        setFirstTime(false);
    }

    if (getVerboseMode()){
        std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "Previous Command: " << msgCommand << std::endl;
    }

    while(inputSignals[0]->ready() || msgCommand == "GET_KEY"){
    
        if (inputSignals[0]->ready()){
            t_message msgReceived;
            inputSignals[0]->bufferGet(&msgReceived);
            receivedMessages.push_back(msgReceived);
            msgJson = json::parse(msgReceived.getMessageData());
            msgCommand = msgJson["command"];
            msgData = msgJson["data"];
        } else if (mode == PULL){
            msgCommand = NULL;
        }

        // verificar comando e enviar resposta

        if (msgCommand == "OPEN_CONNECT"){
            if(getVerboseMode()){
                std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "RECEIVED OPEN_CONNECT" << std::endl;
            }
            
            // Check key_stream_id
            if (!msgData.contains("key_stream_id")) {

                // generate random UUID
                boost::uuids::random_generator generator;
                boost::uuids::uuid uuid = generator();
                etsi_qkd_004::UUID KSID = boost::uuids::to_string(uuid);

                // verify if already exists and generate another UUID if so
                while(Sessions.find(KSID) != Sessions.end()){
                    uuid = generator();
                    KSID = boost::uuids::to_string(uuid);
                }

                SessionInfo info;
                info.keyID_Tx = 0;
                info.keyID_Rx = 0;
                Sessions[KSID] = info;
                setKeyStreamId(KSID);
            } else {
                etsi_qkd_004::UUID KSID = msgData["key_stream_id"];
                if (Sessions.find(KSID) == Sessions.end()) {
                    SessionInfo info;
                    info.keyID_Tx = 0;
                    info.keyID_Rx = 0;
                    Sessions[KSID] = info;
                    setKeyStreamId(KSID);
                } else { status = etsi_qkd_004::OC_KSID_ALREADY_IN_USE;}
            }

            unsigned int key_type = msgData["qos"]["key_type"];
            unsigned int key_chunk_size = msgData["qos"]["key_chunk_size"];
            unsigned int max_bps = msgData["qos"]["max_bps"];
            unsigned int min_bps = msgData["qos"]["min_bps"];
            unsigned int jitter = msgData["qos"]["jitter"];
            unsigned int priority = msgData["qos"]["priority"];
            unsigned int timeout = msgData["qos"]["timeout"];
            unsigned int ttl = msgData["qos"]["ttl"];
            std::string metadata_mimetype = msgData["qos"]["metadata_mimetype"];
            setQoS(key_type, key_chunk_size, max_bps, min_bps, jitter, priority, timeout, ttl, metadata_mimetype);

            // signal with key_type
            if (key_type == 0){
                outputSignals[1]->bufferPut((t_binary)0);
            } else if (key_type == 1){ outputSignals[1]->bufferPut((t_binary)1);}
            
            t_message msgSend;
            t_string msgDataSend = etsi_qkd_004::handle_open_connect(key_stream_id,getQoS(),status).dump();
            msgSend.setMessageData(msgDataSend);
            outputSignals[0]->bufferPut(msgSend);

        } else if (msgCommand == "OPEN_CONNECT_RESPONSE"){
            if(getVerboseMode()){
                std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "RECEIVED OPEN_CONNECT_RESPONSE" << std::endl;
            }

            etsi_qkd_004::Status status_ = msgData["status"];
            if ( status_ == etsi_qkd_004::SUCCESSFUL ){
                etsi_qkd_004::UUID KSID = msgData["key_stream_id"];
                
                SessionInfo info;
                info.keyID_Tx = 0;
                info.keyID_Rx = 0;
                Sessions[KSID] = info;
                setKeyStreamId(KSID);

                t_message msgSend;
                t_string msgDataSend = etsi_qkd_004::get_key(key_stream_id,Sessions[key_stream_id].keyID_Tx++,metadata_client).dump();
                msgSend.setMessageData(msgDataSend);
                outputSignals[0]->bufferPut(msgSend);

            } else if ( status_ == etsi_qkd_004::OC_KSID_ALREADY_IN_USE ){
                if(getVerboseMode()){
                    std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "ERROR: OPEN_CONNECT failed because the Key_stream_ID is already in use." << std::endl;
                }
            }

        } else if (msgCommand == "GET_KEY"){

            setKeyStreamId(msgData["key_stream_id"]);

            if (mode == PULL){

                if(getVerboseMode()){
                    std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "RECEIVED GET_KEY PULL MODE" << std::endl;
                }
                etsi_qkd_004::KeyBuffer keyBuffer;

                if(inputSignals[1]->ready()){
                    for(auto k = 0; k < getQoS().key_chunk_size ; k++){
                        t_binary kval{0};
                        inputSignals[1]->bufferGet(&kval);
                        keyBuffer.push_back(kval);
                    }
                }
                if (getVerboseMode()){
                    std::cout << "keyBuffer: [";
                    for (unsigned char c : keyBuffer) {
                    std::cout << static_cast<int>(c);
                    }
                    std::cout << "]" << std::endl;
                }

                t_string msgDataSend;
                if(!keyBuffer.empty()){
                    status = etsi_qkd_004::SUCCESSFUL;
                    msgDataSend = etsi_qkd_004::handle_get_key(status,keyBuffer,Sessions[key_stream_id].keyID_Rx++,metadata_server).dump();
                } else { 
                    status = etsi_qkd_004::GK_INSUFFICIENT_KEY_AVAILABLE;
                    msgDataSend = etsi_qkd_004::handle_get_key(status,keyBuffer,Sessions[key_stream_id].keyID_Rx,metadata_server).dump();
                }

                t_message msgSend;
                msgSend.setMessageData(msgDataSend);
                outputSignals[0]->bufferPut(msgSend);

            } else if (mode == PUSH){

                if(getVerboseMode()){
                std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "RECEIVED GET_KEY PUSH MODE" << std::endl;
                }
                
                etsi_qkd_004::KeyBuffer keyBuffer;

                if (inputSignals[1]->ready() && outputSignals[0]->space() && Sessions[key_stream_id].keyID_Rx < num_keys){
                    for(auto k = 0; k < getQoS().key_chunk_size ; k++){
                        t_binary kval{0};
                        inputSignals[1]->bufferGet(&kval);
                        keyBuffer.push_back(kval);
                    }
                    if (getVerboseMode()){
                        std::cout << "keyBuffer: [";
                        for (unsigned char c : keyBuffer) {
                        std::cout << static_cast<int>(c);
                        }
                        std::cout << "]" << std::endl;
                    }

                    t_string msgDataSend;
                    if(!keyBuffer.empty()){
                        status = etsi_qkd_004::SUCCESSFUL;
                        msgDataSend = etsi_qkd_004::handle_get_key(status,keyBuffer,Sessions[key_stream_id].keyID_Rx++,metadata_server).dump();
                    } else { 
                        status = etsi_qkd_004::GK_INSUFFICIENT_KEY_AVAILABLE;
                        msgDataSend = etsi_qkd_004::handle_get_key(status,keyBuffer,Sessions[key_stream_id].keyID_Rx,metadata_server).dump();
                    }

                    t_message msgSend;
                    msgSend.setMessageData(msgDataSend);
                    outputSignals[0]->bufferPut(msgSend);

                    break;
                } else {alive = true; break;}
            }  
        } else if (msgCommand == "GET_KEY_RESPONSE"){
            if(getVerboseMode()){
                std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "RECEIVED GET_KEY_RESPONSE" << std::endl;
            }

            etsi_qkd_004::Status status_ = msgData["status"];
            unsigned int key_index = msgData["index"].get<unsigned int>();
            if ( status_ == etsi_qkd_004::SUCCESSFUL ) {
                etsi_qkd_004::KeyBuffer keyBuffer = msgData["key_buffer"].get<etsi_qkd_004::KeyBuffer>();
                metadata_server.size = msgData["metadata"]["size"];
                metadata_server.buffer = msgData["metadata"]["buffer"];

                // keyBuffer to key signal
                for (int i = 0; i < keyBuffer.size(); i++) {
                    outputSignals[1]->bufferPut(keyBuffer[i]);
                }

                // key_index to index signal
                t_message index_;
                index_.setMessageData(std::to_string(key_index));
                outputSignals[2]->bufferPut(index_);

                // create response message
                if (mode == PULL && Sessions[key_stream_id].keyID_Tx < num_keys){
                    t_message msgSend;
                    t_string msgDataSend = etsi_qkd_004::get_key(key_stream_id,Sessions[key_stream_id].keyID_Tx++,metadata_client).dump();
                    msgSend.setMessageData(msgDataSend);
                    outputSignals[0]->bufferPut(msgSend);

                } else if ( key_index == num_keys-1){
                    t_message msgSend;
                    t_string msgDataSend = etsi_qkd_004::close(key_stream_id).dump();
                    msgSend.setMessageData(msgDataSend);
                    outputSignals[0]->bufferPut(msgSend);
                    //setTerminated(true);
                }

            } else if (status_ == etsi_qkd_004::GK_INSUFFICIENT_KEY_AVAILABLE){
                if(getVerboseMode()){
                    std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "ERROR: GET_KEY failed because insufficient key available." << std::endl;
                }

                // create response message with the same index
                t_message msgSend;
                t_string msgDataSend = etsi_qkd_004::get_key(key_stream_id,key_index,metadata_client).dump();
                msgSend.setMessageData(msgDataSend);
                outputSignals[0]->bufferPut(msgSend);

            }
            break;

        } else if (msgCommand == "CLOSE"){
            if(getVerboseMode()){
                std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "RECEIVED CLOSE" << std::endl;
            }

            etsi_qkd_004::UUID KSID = msgData["key_stream_id"];

            // delete session
            Sessions.erase(KSID);

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
            //setFirstTime(true);
            setTerminated(true);
        
        } else if (msgCommand == "CLOSE_RESPONSE"){
            if(getVerboseMode()){
                std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "RECEIVED CLOSE_RESPONSE" << std::endl;

                // delete session
                Sessions.erase(key_stream_id);

                for(const auto& msg : receivedMessages){
                    std::cout << "MSG: " << i << " - " << msg << std::endl;
                    i++;
                }
            }
            setTerminated(true);

        } else {
            std::cout << ((ID == "Tx") ? "[004-SOUTH]: " : "[004-NORTH]: ") << "UNKNOWN COMMAND" << std::endl;
        }
    }
    
    std::cout << "[ETSI_SOUTH]: EXIT" << std::endl;
    return alive;
}