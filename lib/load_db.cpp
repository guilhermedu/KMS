#include "load_db.h"
#include <vector>
#include <string>
#include <iostream>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include "peer_comm.h"


//IS0 -> SessionInfo(key_stream_id, keyType, key_chunk_size)
//IS1 -> NewKey_ACK
//OS0 -> NewKey
//OS1 -> Key

//Access the databse and go,in the right order starting in SEQ=0, select raw_key material and unify each SEQ to form a key
//This key is going to have the size of key_chunk_size
//After i get each SEQ, i need to subtract the key_chunk_size from the size of the key_material
//Each SEQ divided in indexes of size 32
//If the size of the key_material is less than key_chunk_size, i need to get the next SEQ
//I always need to specify the size_used of each index inside a SEQ

// se um recebe um SessionInfo significa que tem de criar uma chave ou ir buscar uma chave já criada
// no signal new_key_ack tanto pode vir um ack como um new_key, depende do papel da app conectada ao kms
// o mesmo acontece para o sinal new_key

void LoadDB::initialize(void){
    setDBConnection(IP,"kms","YourStrongPassword","KeyStorage");
}

bool LoadDB::runBlock(void){

    std::cout << "[LOAD_DB]: ENTER" << std::endl;

    stmt = connection->createStatement();

    std::map<int,std::vector<int>>indexes;
    // verify if new_key/new_key_ack was received (first than sessionInfo)
    if (inputSignals[1]->ready()){
        t_message msg_newKeyAck;
        inputSignals[1]->bufferGet(&msg_newKeyAck);
        json newKeyAckJson = json::parse(msg_newKeyAck.getMessageData());
        msgCommand = newKeyAckJson["command"];
        msgData = newKeyAckJson["data"];

        if (msgCommand == "NEW_KEY"){
            if(getVerboseMode()){
                std::cout << "[LOAD_DB]: NEW_KEY ARRIVED" << std::endl;
            }
            key_stream_id = msgData["key_stream_id"];
            msg_index = msgData["id"];
            key_type = msgData["qos"]["key_type"];
            key_chunk_size = msgData["qos"]["key_chunk_size"];
            std::map<int,std::vector<int>> new_key_ids = msgData["indexes"].get<std::map<int,std::vector<int>>>();
            

            std::string db_raw_name = key_type ? "raw_key_store_oblivious" : "raw_key_store_symmetric";
            std::string db_key_name = key_type ? "key_store_oblivious" : "key_store_symmetric";

            // retrieve key_material with received indexes
            std::string key_material = "";
            for (const auto& pair : new_key_ids) {
                int seq = pair.first;
                for (int id : pair.second) {
                    std::string query = "SELECT key_material FROM " + db_raw_name + " WHERE seq = " + std::to_string(seq) + " AND id = " + std::to_string(id);
                    std::string updateQuery = "UPDATE " + db_raw_name + " SET size_used = 1 WHERE seq = " + std::to_string(seq) + " AND id = " + std::to_string(id);
                    stmt->executeUpdate(updateQuery);
                    res = stmt->executeQuery(query);
                    if (res->next()) {
                        key_material += res->getString("key_material");
                        if (key_material.size() >= key_chunk_size) {
                            key_material = key_material.substr(0, key_chunk_size);
                            break;
                        }
                    }
                }
                if (key_material.size() >= key_chunk_size) {
                    break;
                }
            }

            // insert key created in db
            if (key_material.size() >= key_chunk_size) {
                // Prepare the SQL INSERT statement
                std::string query = "INSERT INTO " + db_key_name + " (KSID, id, creation_timestamp, used, key_material) VALUES ('" + key_stream_id + "', " + std::to_string(msg_index) + ", CURRENT_TIMESTAMP(3), 0, '" + key_material + "' )";
                stmt->executeUpdate(query);
            }

            t_string msgDataSend = key_sync::NEW_KEY_ACK(source,destination,key_stream_id, msg_index).dump();
            t_message msgSend;
            msgSend.setMessageData(msgDataSend);
            outputSignals[0]->bufferPut(msgSend);

        } else if (msgCommand == "NEW_KEY_ACK"){
            if(getVerboseMode()){
                std::cout << "[LOAD_DB]: NEW_KEY_ACK ARRIVED" << std::endl;
            }

            std::string db_raw_name = key_type ? "raw_key_store_oblivious" : "raw_key_store_symmetric";
            std::string db_key_name = key_type ? "key_store_oblivious" : "key_store_symmetric";

            key_stream_id = msgData["key_stream_id"];
            msg_index = msgData["id"];
            key_type = SentMessages[key_stream_id][msg_index].key_type;
            key_chunk_size = SentMessages[key_stream_id][msg_index].key_chunk_size;
            std::map<int,std::vector<int>> new_key_ids = SentMessages[key_stream_id][msg_index].indexes;

            // retrieve key_material with sent indexes and update its size_used (currently if used 1)
            std::string key_material = "";
            for (const auto& pair : new_key_ids) {
                int seq = pair.first;
                for (int id : pair.second) {
                    std::string query = "SELECT key_material FROM " + db_raw_name + " WHERE seq = " + std::to_string(seq) + " AND id = " + std::to_string(id);
                    std::string updateQuery = "UPDATE " + db_raw_name + " SET size_used = 1 WHERE seq = " + std::to_string(seq) + " AND id = " + std::to_string(id);
                    stmt->executeUpdate(updateQuery);
                    res = stmt->executeQuery(query);
                    if (res->next()) {
                        key_material += res->getString("key_material");
                        if (key_material.size() >= key_chunk_size) {
                            key_material = key_material.substr(0, key_chunk_size);
                            break;
                        }
                    }
                }
                if (key_material.size() >= key_chunk_size) {
                    break;
                }
            }

            // insert key created in db
            if (key_material.size() >= key_chunk_size) {
                // Prepare the SQL INSERT statement
                std::string query = "INSERT INTO " + db_key_name + " (KSID, id, creation_timestamp, used, key_material) VALUES ('" + key_stream_id + "', " + std::to_string(msg_index) + ", CURRENT_TIMESTAMP(3), 0, '" + key_material + "' )";
                stmt->executeUpdate(query);
            }

        }
        // Execute the query that retrieves all rows from the key_store_symmetric table
        res = stmt->executeQuery("SELECT * FROM key_store_oblivious");
        // Loop over the result set and print each row
        std::cout << "KEY STORE OBLIVIOUS" << std::endl;
        while (res->next()) {
            std::cout << res->getString("ksid") << ", " << res->getInt("id") << ", " << res->getString("hash") << ", " << res->getString("expiration_timestamp") << ", " << res->getBoolean("suspended") << ", " << res->getString("creation_timestamp") << ", " << res->getBoolean("used") << ", " << res->getString("key_material") << std::endl;
            std::cout << "--------------------------------------------------------------------------------------" << std::endl;
        }
        key_ready = true;
    }

    // send key signal to north if a key was created
    if ( key_ready ){
        std::string db_key_name = key_type ? "key_store_oblivious" : "key_store_symmetric";
        // Check if there is an entry in the key_stored table with the given KSID and msg_id
        std::string query = "SELECT * FROM " + db_key_name + " WHERE ksid = '" + key_stream_id + "' AND id = " + std::to_string(msg_index);
        res = stmt->executeQuery(query);
        if (res->next()) {
            // send key to north etsi
            std::string key_material = res->getString("key_material");
            for (auto val : key_material){
                outputSignals[1]->bufferPut(static_cast<char>(val));
            }
        }
        key_ready = false;
    }

    if (inputSignals[0]->ready()){
        // receive msg
        t_message msg_sessionInfo;
        inputSignals[0]->bufferGet(&msg_sessionInfo);
        sessionInfoJson = json::parse(msg_sessionInfo.getMessageData());
        // retrieve info
        key_stream_id = sessionInfoJson["key_stream_id"];
        msg_index = sessionInfoJson["msg_index"];
        key_type = sessionInfoJson["key_type"];
        key_chunk_size = sessionInfoJson["key_chunk_size"];

        std::string db_key_name = key_type ? "key_store_oblivious" : "key_store_symmetric";
        // Check if there is an entry in the key_stored table with the given KSID and msg_id
        std::string query = "SELECT * FROM " + db_key_name + " WHERE ksid = '" + key_stream_id + "' AND id = " + std::to_string(msg_index);
        res = stmt->executeQuery(query);
        if (res->next()) {
            // send key to north etsi
            std::string key_material = res->getString("key_material");
            for (auto val : key_material){
                outputSignals[1]->bufferPut(static_cast<char>(val));
            }
        } else {
            std::string db_raw_name = key_type ? "raw_key_store_oblivious" : "raw_key_store_symmetric";
            // Retrieve raw material indexes from db
            res = stmt->executeQuery("SELECT * FROM " + db_raw_name + " WHERE sync = 1 AND size_used = 0 ORDER BY seq ASC");
            unsigned int total_size = 0;
            while (res->next() && total_size < key_chunk_size) {
                // Access data
                int seq = res->getInt("seq");
                int id = res->getInt("id");
                int size = res->getInt("size");
                int size_used = res->getInt("size_used");
                total_size += size;
                indexes[seq].push_back(id); 
                // Update size_used in the database
                std::string updateQuery = "UPDATE " + db_raw_name + " SET size_used = 1 WHERE seq = " + std::to_string(seq) + " AND id = " + std::to_string(id);
                stmt->executeUpdate(updateQuery);
            }

            qos.key_chunk_size = key_chunk_size;
            qos.key_type = key_type;

            msgDataSend = key_sync::NEW_KEY(source,destination,key_stream_id,msg_index,qos,indexes).dump();
            msgSend.setMessageData(msgDataSend);
            outputSignals[0]->bufferPut(msgSend);

            NEW_KEY_SENT new_key_sent;
            new_key_sent.key_type = key_type;
            new_key_sent.key_chunk_size = key_chunk_size;
            SentMessages[key_stream_id].push_back(new_key_sent);
            SentMessages[key_stream_id][msg_index].indexes = indexes;

        }
        // delete res;
        // delete stmt;

    }

    std::cout << "[LOAD_DB]: EXIT" << std::endl;

    return true;
}