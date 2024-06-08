#include "ETSI004_south.h"
#include "etsi_qkd_004.h"

void ETSI004South::initialize(void){

    t_string msgDataSend = etsi_qkd_004::open_connect(source, destination, qos, KSID).dump();
    t_message msgSend;
    msgSend.setMessageData(msgDataSend);
    outputSignals[0]->bufferPut(msgSend);
}

bool ETSI004South::runBlock(void){

    bool alive = true;
    if (getTerminated()) return false;

    ready = inputSignals[0]->ready();
    if(ready){
        t_message msgReceived;
        inputSignals[0]->bufferGet(&msgReceived);
        receivedMessages.push_back(msgReceived);
        msgJson = json::parse(msgReceived.getMessageData());
        msgCommand = msgJson["command"];
        msgData = msgJson["data"];

        if (msgCommand == "OPEN_CONNECT_RESPONSE"){
            if(getVerboseMode()){
                std::cout << "[004-SOUTH]: " << "RECEIVED OPEN_CONNECT_RESPONSE" << std::endl;
            }

            etsi_qkd_004::Status check_status = msgData["status"];
            if ( check_status == etsi_qkd_004::SUCCESSFUL ){
                KSID = msgData["key_stream_id"];
                
                SessionInfo info;
                info.num_keys = num_keys;
                info.key_type = qos.key_type;
                info.key_chunk_size = qos.key_chunk_size;
                Sessions[KSID] = info;

                t_message msgSend;
                t_string msgDataSend = etsi_qkd_004::get_key(KSID,0,metadata_client).dump();
                msgSend.setMessageData(msgDataSend);
                outputSignals[0]->bufferPut(msgSend);

            } else if ( check_status == etsi_qkd_004::OC_KSID_ALREADY_IN_USE ){
                if(getVerboseMode()){
                    std::cout << "[004-SOUTH]: " << "ERROR: OPEN_CONNECT failed because the Key_stream_ID is already in use." << std::endl;
                }
            }

        } else if (msgCommand == "GET_KEY_RESPONSE"){
            if(getVerboseMode()){
                std::cout << "[004-SOUTH]: " << "RECEIVED GET_KEY_RESPONSE" << std::endl;
            }

            etsi_qkd_004::Status check_status = msgData["status"];
            unsigned int key_id = msgData["index"].get<unsigned int>();
            if( check_status == etsi_qkd_004::SUCCESSFUL) {
                etsi_qkd_004::KeyBuffer keyBuffer = msgData["key_buffer"].get<etsi_qkd_004::KeyBuffer>();
                // keyBuffer to key signal
                for (int i = 0; i < keyBuffer.size(); i++) {
                    outputSignals[1]->bufferPut(keyBuffer[i]);
                }
                // key_index to index signal
                t_message index_;
                index_.setMessageData(std::to_string(key_id));
                outputSignals[2]->bufferPut(index_);

                if( key_id == Sessions[KSID].num_keys-1 ){
                    t_message msgSend;
                    t_string msgDataSend = etsi_qkd_004::close(KSID).dump();
                    msgSend.setMessageData(msgDataSend);
                    outputSignals[0]->bufferPut(msgSend);
                }

            } else if (check_status == etsi_qkd_004::GK_INSUFFICIENT_KEY_AVAILABLE){
                if(getVerboseMode()){
                    std::cout << "[004-SOUTH]: " << "ERROR: GET_KEY failed because insufficient key available." << std::endl;
                }
            }
        } else if (msgCommand == "CLOSE_RESPONSE"){
            if(getVerboseMode()){
                std::cout << "[004-SOUTH]: " << "RECEIVED CLOSE_RESPONSE" << std::endl;
                for(const auto& msg : receivedMessages){
                    std::cout << "MSG: " << i << " - " << msg << std::endl;
                    i++;
                }
            }
        } else {std::cout << "[004-SOUTH]: " << "RECEIVED UNRECOGNIZED COMMAND" << std::endl;}
    }

    return true;
}