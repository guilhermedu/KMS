#ifndef SAVE_DB_H
#define SAVE_DB_H

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

using json = nlohmann::ordered_json;

class SaveDB: public Block {
public:
    SaveDB(std::initializer_list<Signal *> InputSig, std::initializer_list<Signal *> OutputSig) :Block(InputSig, OutputSig) {};
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
    void setKeyType(unsigned int keyType){
        kType = keyType;
    };

    void setIPDB(std::string ip){
        IP = ip;
    };  

    void setSaveType(int type){
        saveType = type;
    }
    int getSaveType(){
        return saveType;
    }

private:
    sql::Driver* driver;
    sql::Connection* connection;
    sql::Statement* stmt;
    sql::ResultSet* res;
    std::string IP;
    unsigned int kType{0}; // 0-SYMMETRIC, 1-OBLIVIOUS
    int saveType{0}; // 0-ASCII, 1-B64
    int part_size{32};  // Set the size of each part of key_material
};

#endif //SAVE_DB_H
