# define PROGRAM_LOAD_REQUEST_H_

#include "netxpto_20200819.h"

class LoadRequest : public Block {

public:

    LoadRequest(std::initializer_list<Signal *> InputSig, std::initializer_list<Signal *> OutputSig) :Block(InputSig, OutputSig) {};

    void initialize(void);
    bool runBlock(void);

    enum class delimiter_type { CommaSeperatedValues, ConcatenatedValues };

    void setSamplingPeriod(double sPeriod) { samplingPeriod = sPeriod; }
    double const getSamplingPeriod(void) { return samplingPeriod; }

    void setSymbolPeriod(double sPeriod) { symbolPeriod = sPeriod; }
    double const getSymbolPeriod(void) { return symbolPeriod; }

    void setDataType(signal_value_type dType) { dataType = dType; }
    signal_value_type const getDataType(void) { return dataType; }

    void setDelimiterType(delimiter_type delType) { delimiterType = delType; }
    delimiter_type const getDelimiterType(void) { return delimiterType; }

    void setRequest(t_string req) {request = req; }
    t_string getRequest(void) {return request; }

    // void setCycleModulos(bool cycle) { cycleModulos = cycle; }
    // bool getCycleModulos() { return cycleModulos; }

    // void setSartColumn(int sColumn) { startColumn = sColumn; }
    // int const getStartColumn(void) { return startColumn; }
    
    // void setSartLine(int sLine) { startLine = sLine; }
    // int const getStartLine(void) { return startLine; }

    // void setMultiplyValue(double aMultiply) { auxMultiply = aMultiply; }
    // double const getMultiplyValue(void) { return auxMultiply; }

    unsigned long int getNumberOfLoadedValues(void) const { return numberOfLoadedValues; }

private:

std::streampos position = 0;

//unsigned long int index = 0;

int ready{ 0 };
double samplingPeriod{ 1 };
double symbolPeriod{ 1 };
double auxMultiply = 1;
signal_value_type dataType{ signal_value_type::t_char };
delimiter_type delimiterType{ delimiter_type::ConcatenatedValues };
t_string request = "{\"command\":\"OPEN_CONNECT\",\"data\":{\"source\":\"source\",\"destination\":\"destination\",\"qos\":{\"key_chunk_size\":3,\"max_bps\":5,\"min_bps\":1,\"jitter\":4,\"priority\":5,\"timeout\":0,\"ttl\":10,\"metadata_mimetype\":\"metadata\"},\"key_stream_id\":\"key_stream_id\"}}";


bool cycleModulos = true; // mandates if the program should read files on loop (module)


unsigned long int startColumn = 1;
unsigned long int startLine = 1;
unsigned long int auxPosition = 0;

unsigned long int numberOfLoadedValues{ 0 };
};