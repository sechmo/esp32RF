#include <RFSync.h>


// 4 bit to 6 bit symbol converter table
// Used to convert the high and low nybbles of the transmitted data
// into 6 bit symbols for transmission. Each 6-bit symbol has 3 1s and 3 0s 
// with at most 3 consecutive identical bits
IRAM_ATTR static uint8_t symbols[] =
{
    0xd,  0xe,  0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c, 
    0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34
};

IRAM_ATTR uint8_t symbol_6to4(uint8_t symbol) {
    for (uint8_t i = 0; i < sizeof(symbols); i++) {
        if (symbols[i] == symbol) {
            return i;
        }
    }
    return 0;
}






void RFSync::registerSample(bool sample)
{
    _rxCurrentSample = sample;
    if (sample) {
        _rxSampleCount++;
    }
}

void RFSync::synchronize()
{
    if (_rxCurrentSample != _rxLastSample)
    {
        // probable bit transition detected

        if (_rxPeriodProgress < _bitPeriodTransition)
        {
            // we changed earlier than expected, lets 
            // retard the progress counter making it 
            // closer to sync with the start of the 
            // current period
            _rxPeriodProgress += _bitPeriodRetard;
        }
        else {
            // we changed and were already closer to 
            // the counter's end, lets try to catch it
            // to sync with the start of the previous cycle
            _rxPeriodProgress += _bitPeriodAdvance;
        }
    }
    else
    {
        // probably still sampling same bit
        _rxPeriodProgress += _bitPeriodIncrement;
    }
}

bool RFSync::bitTransition()
{
    return _rxPeriodProgress >= _bitPeriodLength;
}

void RFSync::processBit()
{

    // Lets restart the progress keeping the bias
    // added for syncronization
    _rxPeriodProgress -= _bitPeriodLength;


    // open place for the new bit,
    // as we receive least significat bit first
    // we move bits down
    _rxLast12EncodedBits >>= 1;

    // majority check over the samples to 
    // determine if it was 1 or 0
    if (_rxSampleCount > 4)
    {
        _rxLast12EncodedBits |= 0x800;
    }

    // reset the sample counter
    _rxSampleCount = 0;



    if (_rxActive) {
        // if we were already receiving a message

        _rxEncodedBitCount++;

        if (_rxEncodedBitCount == 12) {
            // we have the last 12 bits of the message
            // lets decode them

            uint8_t receivedByte = 
            (symbol_6to4(_rxLast12EncodedBits & 0x3f)) << 4 
            | symbol_6to4(_rxLast12EncodedBits >> 6);


            // first byte is the byte count of the incoming msg
            if (_rxBufferLen == 0) {
                _rxMsgSize = receivedByte;

                if (_rxMsgSize < 7 || _rxMsgSize > _rxBufferSize) {
                    // stupid message length, drop the whole thing
                    _rxActive = false;
                    return;
                }
            }
            _rxBuffer[_rxBufferLen++] = receivedByte;

            if (_rxBufferLen >= _rxMsgSize) {
                // got all the bytes now

                _rxBufferFull = true;
                _rxActive = false;

                disable();
            }

            // reset the bit counter
            _rxEncodedBitCount = 0;
        }

    }
    else if (_rxLast12EncodedBits == _startSymbol)
    {
        // if we have the start symbol
        _rxActive = true;
        _rxEncodedBitCount = 0;
        _rxBufferLen = 0;
    }



}