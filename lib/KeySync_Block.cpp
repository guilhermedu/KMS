#include "KeySync_Block.h"
#include "peer_comm.h"
#include "netxpto_20200819.h"

void KeySyncBlock::initialize(void) {

}

bool KeySyncBlock::runBlock(void){
    
    bool alive = true;
    bool currentIndexMentioned = false;
    std::set<unsigned int> indexesToErase;
    if (getTerminated()) return false;

    std::cout << "[KEY_SYNC_BLOCK]: ENTER" << std::endl;

    // fill the indexes vector with own indexes
    ready = inputSignals[1]->ready();
    if (ready){
        inputSignals[1]->bufferRead(&m_index);
        indexes.push_back(m_index.getMessageData());

        // Update currentIndex
        currentIndex = std::stoul(indexes.back());
    }

    if (inputSignals[2]->ready()){
        // send new_key to peer
        t_message north_new_key;
        inputSignals[2]->bufferGet(&north_new_key);
        outputSignals[0]->bufferPut(&north_new_key);
        std::cout << "[KEY_SYNC_BLOCK]: SENT NEW_KEY: " << std::endl;
    }

    // check peer message
    if (inputSignals[0]->ready()){
        t_message msgReceived;
        inputSignals[0]->bufferGet(&msgReceived);
        receivedMessages.push_back(msgReceived);
        msgJson = json::parse(msgReceived.getMessageData());
        msgCommand = msgJson["command"];
        msgData = msgJson["data"];

        if (msgCommand == "KEY_SYNC"){
            if(getVerboseMode()){
                std::cout << "RECEIVED KEY_SYNC" << std::endl;
            }
            //save the received indexes
            key_sync::IndexBuffer r_indexes = msgData["indexBuffer"].get<key_sync::IndexBuffer>();
            for (const auto& index : r_indexes) {
                receivedIndexes.insert(index);
                r_IndexesInt.insert(std::stoul(index));
                lastReceivedIndex = std::stoul(index);
            }

            // check if there are missing indexes
            auto it = r_IndexesInt.begin();
            auto next_it = std::next(it);

            while (next_it != r_IndexesInt.end()) { 
                int dif = *next_it - *it;
                if (dif != 1) {
                    for (int i = 1; i < dif; i++)
                    {
                        unsigned int discardedIndex = *it + i;
                        NotFoundIndexes.insert(discardedIndex);
                        std::cout << "INDEX NOT FOUND: " << discardedIndex << std::endl;
                    }
                    
                }
                it = next_it;
                ++next_it;
            }

        } else if (msgCommand == "NEW_KEY" || msgCommand == "NEW_KEY_ACK"){
            // send new_key/new_key_ack to north
            std::cout << "[KEY_SYNC_BLOCK]: RECEIVED: " << msgCommand << std::endl;
            outputSignals[3]->bufferPut(msgReceived);
        } 
    }
    // check which of the received indexes were already sent
    for (const auto& index : receivedIndexes){
        if (sentIndexes.find(index) != sentIndexes.end()){
            sync_indexes.push_back(std::stoul(index));
        }
    }

    // send message to peer
    if (indexes.size() == 10){
        key_sync::Status status = key_sync::Status::SUCCESSFUL;

        t_string msgDataSend = key_sync::KEY_SYNC(source,destination,status,indexes,qos).dump();
        t_message msgSend;
        msgSend.setMessageData(msgDataSend);
        outputSignals[0]->bufferPut(msgSend);
        sentIndexes.insert(indexes.begin(), indexes.end());
        indexes.clear();
        std::cout << "SYNC TO PEER: " << msgDataSend << std::endl;
    }

    // send signal to database to set sync true
    if(!sync_indexes.empty()){
        t_string msgDataSend = key_sync::SYNC_INDEX(sync_indexes).dump();
        t_message msgSend;
        msgSend.setMessageData(msgDataSend);
        outputSignals[1]->bufferPut(msgSend);
        sync_indexes.clear();
        std::cout << "SIGNAL TO SYNC DATABASE: " << msgDataSend << std::endl;
    }

    // check which of the indexes to discard exist in the KMS
    for (const auto& index : NotFoundIndexes){
        if (sentIndexes.find(std::to_string(index)) == sentIndexes.end()){
            indexesToErase.insert(index);
        }
    }
    // erase indexes that dont exist in the KMS
    for (const auto& index : indexesToErase) {
        NotFoundIndexes.erase(index);
    }

    
    for (const auto& index : NotFoundIndexes){
        discardIndexes.push_back(index);
    }

    // send message to database to discard indexes
    t_string msgDataSend = key_sync::DISCARD(discardIndexes).dump();
    t_message msgSend;
    msgSend.setMessageData(msgDataSend);
    outputSignals[2]->bufferPut(msgSend);
    std::cout << "SIGNAL TO DISCARD INDEXES DATABASE: " << msgDataSend << std::endl;
    discardIndexes.clear();

    std::cout << "[KEY_SYNC_BLOCK]: EXIT" << std::endl;
    return alive;

}

//////////////////////////// NOT USED //////////////////////////////////////


/*     // check if the last index was mentioned in the received indexes  
    if (!receivedIndexes.empty()){ 
        if (!currentIndexMentioned && lastReceivedIndex > currentIndex){
            std::cout << "DISCARDED INDEX: " << currentIndex << std::endl;
            t_string msgDataSend = std::to_string(currentIndex);
            t_message msgSend;
            msgSend.setMessageData(msgDataSend);
            outputSignals[2]->bufferPut(msgSend);
        }
    } */