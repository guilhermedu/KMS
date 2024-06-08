#include "load_request.h"

void LoadRequest::initialize(void){
    outputSignals[0]->setSymbolPeriod(symbolPeriod);
    outputSignals[0]->setSamplingPeriod(samplingPeriod);
}

bool LoadRequest::runBlock(void){
    // int space = outputSignals[0]->space();
    // signal_value_type sTypeOut = outputSignals[0]->getValueType();

    // int requestSize= request.size();

    if (getFirstTime()){
        setFirstTime(false);
        t_message msg;
        msg.setMessageData(request);
        outputSignals[0]->bufferPut(msg);
    }

    return false;
    
    // if(getFirstTime()){
    //     setFirstTime(false);
    //     ready = requestSize;
    // }
    // if (ready == 0){
    //     return false;
    // }
    

    // for (t_char c : request){
    //     ready--;
    //     outputSignals[0]->bufferPut(c);
    // }
    
}