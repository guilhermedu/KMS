#ifndef LOAD_DB_H
#define LOAD_DB_H

#include <stdlib.h>
#include <iostream>
#include "mysql_connection.h"
#include <fstream>
#include <string>  
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include "netxpto_20200819.h"
#include "json.hpp"
#include "peer_comm.h"

using json = nlohmann::ordered_json;

class LoadDB: public Block {
public:
    LoadDB(std::initializer_list<Signal *> InputSig, std::initializer_list<Signal *> OutputSig) :Block(InputSig, OutputSig) {};
    void initialize(void);
    bool runBlock(void);

    void setDBConnection(const std::string& host, const std::string& user, const std::string& password, const std::string& database){
        try {
            // Initialize MySQL Connector/C++
            driver = get_driver_instance();

            // Create a connection
            connection = driver->connect("tcp://" + host, user, password);
            connection->setSchema(database);
        } catch (sql::SQLException& e) {
            // Handle exceptions
            std::cerr << "SQLException: " << e.what() << std::endl;
        }
    };

    void setIPDB(std::string ip){
        IP = ip;
    }; 

private:
    sql::Driver* driver;
    sql::Connection* connection;
    sql::Statement* stmt;
    sql::ResultSet* res;
    std::string IP;

    struct NEW_KEY_SENT{
        unsigned int key_type;
        unsigned int key_chunk_size;
        std::map<int,std::vector<int>> indexes;
    };
    std::map<key_sync::UUID,std::vector<NEW_KEY_SENT>> SentMessages;
    
    // temporary variables inside cpp
    key_sync::UUID key_stream_id;
    unsigned int msg_index;
    unsigned int key_type;
    unsigned int key_chunk_size;

    bool key_ready{ false };
    
    t_string msgDataSend;
    t_message msgSend;

    json msgData;
    json msgCommand;
    
    
    key_sync::QoS qos;

    json sessionInfoJson;

    t_string source = "source";
    t_string destination = "destination";


    unsigned int kType{0}; // 0-SYMMETRIC, 1-OBLIVIOUS
    int saveType{0}; // 0-ASCII, 1-B64
    int part_size{32};  // Set the size of each part of key_material
};

#endif //SAVE_DB_H
