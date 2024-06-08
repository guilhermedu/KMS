#include "save_db.h"
#include <vector>
#include <string>
#include <iostream>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

void SaveDB::initialize(void){
    setDBConnection(IP,"kms","YourStrongPassword","KeyStorage");
}

bool SaveDB::runBlock(void){

    std::cout << "[SAVE_DB]: ENTER" << std::endl;

    int ready = inputSignals[0]->ready(); //keys
    auto process = ready;
    stmt = connection->createStatement();

    std::string vals; // string to store keys

    switch(kType){
        case 0: // SYMMETRIC
            if (saveType) { // b64
                for(auto k = 0; k < process; k++)
                {
                    t_binary val{0};
                    inputSignals[0]->bufferGet(&val);
                    vals.push_back(static_cast<char>(val));
                }
            } else {    // ASCII   
                for(auto k = 0; k < process; k++)
                {
                    t_binary val{0};
                    inputSignals[0]->bufferGet(&val);
                    if (val == 0) vals.push_back('0');
                    if (val == 1) vals.push_back('1');
                    vals.push_back(val);
                }
            }

            // Insert keys into db
            if(inputSignals[1]->ready()){
                t_message id;
                inputSignals[1]->bufferGet(&id);
                std::string seq = id.getMessageData();
                
                for (int i = 0; i < vals.size(); i += part_size) {
                    std::string key_material = vals.substr(i, part_size);
                    int size = key_material.size();
                    int Index = i / part_size;
                    stmt->execute("INSERT INTO raw_key_store_symmetric (seq, id, sync, timestamp, size, size_used, key_material) VALUES ('" + seq + "', " + std::to_string(Index) + ", 0, CURRENT_TIMESTAMP(3), " + std::to_string(size) + ", 0, '" + key_material + "')");
                }
            }

            // check if there are indexes to sync and change sync value
            if(inputSignals[2]->ready()){
                t_message sync;
                inputSignals[2]->bufferGet(&sync);
                json sync_json = json::parse(sync.getMessageData());
                std::vector<unsigned int> sync_indexes = sync_json["indexes"].get<std::vector<unsigned int>>();
                for (unsigned int seq : sync_indexes) {
                    std::string query = "UPDATE raw_key_store_symmetric SET sync = 1 WHERE seq = " + std::to_string(seq);
                    stmt->execute(query);
                }
            }

            // check if there are indexes to discard
            if(inputSignals[3]->ready()){
                t_message discard;
                inputSignals[3]->bufferGet(&discard);
                json discard_json = json::parse(discard.getMessageData());
                std::vector<unsigned int> discard_indexes = discard_json["indexes"].get<std::vector<unsigned int>>();
                for (unsigned int seq : discard_indexes) {
                    std::string query = "DELETE FROM raw_key_store_symmetric WHERE seq = " + std::to_string(seq);
                    stmt->execute(query);
                }
            }

            res = stmt->executeQuery("SELECT * FROM raw_key_store_symmetric");
            while (res->next()) {
                // Access data
                std::cout << "\tRaw Key Store Symmetric Record:" << std::endl;
                std::cout << "\tseq: " << res->getInt("seq") << std::endl;
                std::cout << "\tid: " << res->getInt("id") << std::endl;
                std::cout << "\tsync: " << res->getBoolean("sync") << std::endl;
                std::cout << "\ttimestamp: " << res->getString("timestamp") << std::endl;
                std::cout << "\tsize: " << res->getInt("size") << std::endl;
                std::cout << "\tsize_used: " << res->getInt("size_used") << std::endl;
                std::cout << "\tkey_material: " << res->getString("key_material") << std::endl;
            }
            delete res;
            //delete stmt;
            
            break;

        case 1: // OBLIVIOUS
            if (saveType) { // b64
                for(auto k = 0; k < process; k++)
                {
                    t_binary val{0};
                    inputSignals[0]->bufferGet(&val);
                    vals.push_back(static_cast<char>(val));
                }
            } else {    // ASCII   
                for(auto k = 0; k < process; k++)
                {
                    t_binary val{0};
                    inputSignals[0]->bufferGet(&val);
                    if (val == 0) vals.push_back('0');
                    if (val == 1) vals.push_back('1');
                    vals.push_back(val);
                }
            }

            // Insert keys into db
            if(inputSignals[1]->ready()){
                t_message id;
                inputSignals[1]->bufferGet(&id);
                std::string seq = id.getMessageData();
                
                for (int i = 0; i < vals.size(); i += part_size) {
                    std::string key_material = vals.substr(i, part_size);
                    int size = key_material.size();
                    int Index = i / part_size;
                    stmt->execute("INSERT INTO raw_key_store_oblivious (seq, id, sync, timestamp, size, size_used, key_material) VALUES ('" + seq + "', " + std::to_string(Index) + ", 0, CURRENT_TIMESTAMP(3), " + std::to_string(size) + ", 0, '" + key_material + "')");
                }
            }

            // check if there are key_material to sync and change sync value
            if(inputSignals[2]->ready()){
                t_message sync;
                inputSignals[2]->bufferGet(&sync);
                json sync_json = json::parse(sync.getMessageData());
                std::vector<unsigned int> sync_indexes = sync_json["indexes"].get<std::vector<unsigned int>>();
                for (unsigned int seq : sync_indexes) {
                    std::string query = "UPDATE raw_key_store_oblivious SET sync = 1 WHERE seq = " + std::to_string(seq);
                    stmt->execute(query);
                }
            }

            // check if there are indexes to discard
            if(inputSignals[3]->ready()){
                t_message discard;
                inputSignals[3]->bufferGet(&discard);
                json discard_json = json::parse(discard.getMessageData());
                std::vector<unsigned int> discard_indexes = discard_json["indexes"].get<std::vector<unsigned int>>();
                for (unsigned int seq : discard_indexes) {
                    std::string query = "DELETE FROM raw_key_store_oblivious WHERE seq = " + std::to_string(seq);
                    stmt->execute(query);
                }
            }

            // res = stmt->executeQuery("SELECT * FROM raw_key_store_oblivious");
            // while (res->next()) {
            //     // Access data
            //     std::cout << "\tRaw Key Store Oblivious Record:" << std::endl;
            //     std::cout << "\tseq: " << res->getInt("seq") << std::endl;
            //     std::cout << "\tid: " << res->getInt("id") << std::endl;
            //     std::cout << "\tsync: " << res->getBoolean("sync") << std::endl;
            //     std::cout << "\ttimestamp: " << res->getString("timestamp") << std::endl;
            //     std::cout << "\tsize: " << res->getInt("size") << std::endl;
            //     std::cout << "\tsize_used: " << res->getInt("size_used") << std::endl;
            //     std::cout << "\tkey_material: " << res->getString("key_material") << std::endl;
            //     std::cout << "\t------------------------------------------------" << std::endl;
            // }
            //delete res;
            //delete stmt;
            
            break;
        default:
            break;
    }

    std::cout << "[SAVE_DB]: EXIT" << std::endl;

    return true;
}