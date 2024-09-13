#include <RF.h>

class RFSync : public RFReceiver
{

public: 
    RFSync(uint16_t speed = 2000, uint8_t rxPin = 11, uint16_t samples = 8,size_t rxBufferSize = 67, uint16_t startSymbol = 0xb38, uint8_t bitPeriodLength = 160, uint8_t syncBias = 9);


    virtual bool available() override;
    IRAM_ATTR virtual bool receive(uint8_t* buf, size_t *len) override;

    virtual size_t maxMessageLength() override;

private:

    const uint8_t _bitPeriodLength;
    const uint8_t _bitPeriodIncrement;
    const uint8_t _bitPeriodAdvance;
    const uint8_t _bitPeriodRetard;
    const uint8_t _bitPeriodTransition;

    const uint16_t _startSymbol;

    const size_t _rxBufferSize;
    uint8_t* _rxBuffer;
    volatile size_t _rxBufferLen;
    volatile size_t _rxMsgSize;
    volatile bool _rxBufferFull;

    volatile uint8_t _rxCurrentSample;
    volatile uint8_t _rxLastSample;

    // counts the number of positive samples
    volatile uint8_t _rxSampleCount;

    volatile uint8_t _rxPeriodProgress;

    volatile uint8_t _rxActive;

    volatile uint16_t _rxLast12EncodedBits; 
    // how many bits of _rxLast12EncodedBits we have set so far
    volatile uint8_t _rxEncodedBitCount;

    IRAM_ATTR uint8_t symbol6to4(uint8_t symbol);

protected:

    void IRAM_ATTR registerSample(bool sample) override;
    void IRAM_ATTR synchronize() override;
    bool IRAM_ATTR bitTransition() override;
    void IRAM_ATTR processBit() override;

};