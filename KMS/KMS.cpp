#include "load_ascii_2024.h"
#include "netxpto_20200819.h"
#include <typeinfo>
#include <iostream>
#include <memory>
#include "message_handler.h"
#include "save_db.h"
#include "ms_windows_console_output_common_20200819.h"
#include "ip_tunnel_ms_windows_20200819.h"
#include "etsi_qkd_004.h"
#include "cv_qokd_ldpc_multi_machine_sdf.h"
#include "peer_comm.h"
#include "KeySync_Block.h"
#include "KMS.h"
#include "ETSI004_north.h"
#include "load_db.h"
#include "ETSI004_south.h"

DvQkdLdpcInputParameters param = DvQkdLdpcInputParameters();

namespace SOUTH {

    SaveDB saveKeys{{&key, &index, &KeySync::index, &KeySync::discardIndex},{}};
    DestinationTranslationTable dttRxTransmitter;
    MessageHandler MessageHandlerRX{{&response_},{&response}};
    InputTranslationTable ittTxTransmitter;
    MessageHandler MessageHandlerTX{{&request},{&request_}};
    IPTunnel IPTunnel_Client{{&request_},{}};
    IPTunnel IPTunnel_Server{{},{&response_}};
    ETSI004South ETSI004{{&response},{&request, &key, &index}};

    void setup(DvQkdLdpcInputParameters& parameters){
        
        saveKeys.setIPDB(parameters.dbIpAddress);
        saveKeys.setSaveType(parameters.saveType);
        saveKeys.setKeyType(parameters.keyType);

        dttRxTransmitter.add("KMS_South", 0);
        MessageHandlerRX = MessageHandler{{&response_},{&response}, dttRxTransmitter, FUNCTIONING_AS_RX};
        ittTxTransmitter.add(0, {"Key_Provider", "KMS_South"});
        MessageHandlerTX = MessageHandler{{&request},{&request_}, FUNCTIONING_AS_TX, ittTxTransmitter};

        IPTunnel_Client.setLocalMachineIpAddress(parameters.sthIpAddress);
        IPTunnel_Client.setRemoteMachineIpAddress(parameters.kpsIpAddress);
        IPTunnel_Client.setRemoteMachinePort(parameters.kpsPort);
        IPTunnel_Client.setVerboseMode(parameters.verboseMode);
        IPTunnel_Client.setTimeIntervalSeconds(10);

        IPTunnel_Server.setLocalMachineIpAddress(parameters.sthIpAddress);
        IPTunnel_Server.setRemoteMachineIpAddress(parameters.kpsIpAddress);
        IPTunnel_Server.setLocalMachinePort(parameters.sthPort);
        IPTunnel_Server.setVerboseMode(parameters.verboseMode);
        IPTunnel_Server.setTimeIntervalSeconds(10);


        ETSI004.setSource(parameters.etsiSource);
        ETSI004.setDestination(parameters.etsiDest);
        ETSI004.setQoS((unsigned int) parameters.keyType, (unsigned int) parameters.keyChunkSize, (unsigned int) parameters.maxBps, (unsigned int) parameters.minBps, (unsigned int) parameters.jitter, (unsigned int) parameters.priority, (unsigned int) parameters.timeout, (unsigned int) parameters.ttl, parameters.metaMimetype );
        ETSI004.setNumKeys((unsigned int) parameters.numKeys);
        ETSI004.setVerboseMode(parameters.verboseMode);
    }
}

namespace NORTH {

    LoadDB loadDB{{&session_info, &KeySync::new_key_ack},{&new_key,&key}};
    IPTunnel IPTunnel_Server{{},{&request_}};
    DestinationTranslationTable dttRxTransmitter;
    MessageHandler MessageHandlerRX{{&request_},{&request}};
    InputTranslationTable ittTxTransmitter;
    MessageHandler MessageHandlerTX{{&response},{&response_}};
    IPTunnel IPTunnel_Client{{&response_},{}};
    ETSI004North ETSI004{{&request, &key},{&response, &session_info}};

    void setup(DvQkdLdpcInputParameters& parameters) {

        loadDB.setIPDB(parameters.dbIpAddress);
        // readKeys.setAsciiFileNameTailNumber("0");
        // if(param.fileType) readKeys.setAsciiFileNameExtension(".b64");

        IPTunnel_Server.setLocalMachineIpAddress(parameters.nthIpAddress);
        IPTunnel_Server.setRemoteMachineIpAddress(parameters.appIpAddress);
        IPTunnel_Server.setLocalMachinePort(parameters.nthPort);
        IPTunnel_Server.setVerboseMode(parameters.verboseMode);

        IPTunnel_Client.setLocalMachineIpAddress(parameters.nthIpAddress);
        IPTunnel_Client.setRemoteMachineIpAddress(parameters.appIpAddress);
        IPTunnel_Client.setRemoteMachinePort(parameters.appPort);
        IPTunnel_Client.setVerboseMode(parameters.verboseMode);

        dttRxTransmitter.add("KMS_North", 0);
        MessageHandlerRX = MessageHandler{{&request_},{&request}, dttRxTransmitter, FUNCTIONING_AS_RX};
        ittTxTransmitter.add(0, {"APP_A", "KMS_North"});
        MessageHandlerTX = MessageHandler{{&response},{&response_}, FUNCTIONING_AS_TX, ittTxTransmitter };

        ETSI004.setVerboseMode(parameters.verboseMode);
    }
}

namespace KeySync {

    DestinationTranslationTable dttRxTransmitter;
    MessageHandler MessageHandlerRX{{&response_},{&response}};
    InputTranslationTable ittTxTransmitter;
    MessageHandler MessageHandlerTX{{&request},{&request_}};
    IPTunnel IPTunnel_Client{{&request_},{}};
    IPTunnel IPTunnel_Server{{},{&response_}};
    KeySyncBlock KeySync{{&response,&SOUTH::index,&NORTH::new_key},{&request, &index, &discardIndex, &new_key_ack}};

    void setup(DvQkdLdpcInputParameters& parameters, t_string role){

        if(role == "a"){
            dttRxTransmitter.add("KMS_A", 0);
            ittTxTransmitter.add(0, {"KMS_B", "KMS_A"});

            IPTunnel_Client.setLocalMachineIpAddress(parameters.syncAipAddress);
            IPTunnel_Client.setRemoteMachineIpAddress(parameters.syncBipAddress);
            IPTunnel_Client.setRemoteMachinePort(parameters.syncBport);

            IPTunnel_Server.setLocalMachineIpAddress(parameters.syncAipAddress);
            IPTunnel_Server.setRemoteMachineIpAddress(parameters.syncBipAddress);
            IPTunnel_Server.setLocalMachinePort(parameters.syncAport);
            
        } else if (role == "b"){
            dttRxTransmitter.add("KMS_B", 0);
            ittTxTransmitter.add(0, {"KMS_A", "KMS_B"});

            IPTunnel_Client.setLocalMachineIpAddress(parameters.syncBipAddress);
            IPTunnel_Client.setRemoteMachineIpAddress(parameters.syncAipAddress);
            IPTunnel_Client.setRemoteMachinePort(parameters.syncAport);

            IPTunnel_Server.setLocalMachineIpAddress(parameters.syncBipAddress);
            IPTunnel_Server.setRemoteMachineIpAddress(parameters.syncAipAddress);
            IPTunnel_Server.setLocalMachinePort(parameters.syncBport);
        }

        IPTunnel_Client.setVerboseMode(true);
        IPTunnel_Server.setVerboseMode(true);

        MessageHandlerRX = MessageHandler{{&response_},{&response}, dttRxTransmitter, FUNCTIONING_AS_RX};
        MessageHandlerTX = MessageHandler{{&request},{&request_}, FUNCTIONING_AS_TX, ittTxTransmitter};
    }
}

int main(int argc, char *argv[]){

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " a/b\n";
        return 1;
    }
    std::string role = argv[1];
    
    param.setInputParametersFileName("input_KMS.txt");
    param.readSystemInputParameters();
    
    SOUTH::setup(param);
    NORTH::setup(param);
    KeySync::setup(param, role);

    System System_
            {
                {
                &SOUTH::saveKeys,
                &SOUTH::ETSI004,
                &SOUTH::IPTunnel_Client,
                &SOUTH::IPTunnel_Server,
                &SOUTH::MessageHandlerTX,
                &SOUTH::MessageHandlerRX,
                &KeySync::KeySync,
                &KeySync::IPTunnel_Client,
                &KeySync::IPTunnel_Server,
                &KeySync::MessageHandlerTX,
                &KeySync::MessageHandlerRX,
                &NORTH::loadDB,
                &NORTH::IPTunnel_Server,
                &NORTH::ETSI004,
                &NORTH::IPTunnel_Client,
                &NORTH::MessageHandlerTX,
                &NORTH::MessageHandlerRX,
                }
            };

    if (role == "a"){
        System_.setSignalsFolderName("SignalsA");
    } else {System_.setSignalsFolderName("SignalsB");}

    System_.run();
    System_.terminate();

    return 0;
}