#include <RF.h>

class RFSync : public RFReceiver
{

private:

    const uint8_t _bitPeriodLength;
    const uint8_t _bitPeriodIncrement;
    const uint8_t _bitPeriodAdvance;
    const uint8_t _bitPeriodRetard;
    const uint8_t _bitPeriodTransition;

    const uint8_t _startSymbol;

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

    volatile uint8_t _rxLast12EncodedBits; 
    // how many bits of _rxLast12EncodedBits we have set so far
    volatile uint8_t _rxEncodedBitCount;


protected:

    void registerSample(bool sample) override;
    void synchronize() override;
    bool bitTransition() override;
    void processBit() override;

};