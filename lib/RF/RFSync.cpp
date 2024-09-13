#include <RFSync.h>

// 4 bit to 6 bit symbol converter table
// Used to convert the high and low nybbles of the transmitted data
// into 6 bit symbols for transmission. Each 6-bit symbol has 3 1s and 3 0s
// with at most 3 consecutive identical bits
// IRAM_ATTR static uint8_t symbols[] =
static uint8_t symbols[] =
    {
        0xd, 0xe, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c,
        0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34};

// IRAM_ATTR uint8_t RFSync::symbol6to4(uint8_t symbol)
uint8_t RFSync::symbol6to4(uint8_t symbol)
{
    volatile uint8_t i;
    for (i = 0; i < 16; i++)
    {
        if (symbols[i] == symbol)
        {
            return i;
        }
    }

    return 0;


    // uint8_t i;
    // uint8_t count;
    
    // Linear search :-( Could have a 64 byte reverse lookup table?
    // There is a little speedup here courtesy Ralph Doncaster:
    // The shortcut works because bit 5 of the symbol is 1 for the last 8
    // symbols, and it is 0 for the first 8.
    // So we only have to search half the table
    // for (i = (symbol>>2) & 8, count=8; count-- ; i++)
	// if (symbol == symbols[i]) return i;

    return 0; // Not found
}

RFSync::RFSync(
    uint16_t speed,
    uint8_t rxPin,
    uint16_t samples,
    size_t rxBufferSize,
    uint16_t startSymbol,
    uint8_t bitPeriodLength,
    uint8_t syncBias
    )
    : RFReceiver(speed, rxPin, samples),
      _bitPeriodLength(bitPeriodLength),
      _startSymbol(startSymbol),
      _rxBufferSize(rxBufferSize),
      _rxBuffer(new uint8_t[rxBufferSize]),
      _rxBufferLen(0),
      _rxMsgSize(0),
      _rxBufferFull(false),
      _rxCurrentSample(0),
      _rxLastSample(0),
      _rxSampleCount(0),
      _rxPeriodProgress(0),
      _rxActive(false),
      _rxLast12EncodedBits(0),
      _rxEncodedBitCount(0),
      _bitPeriodIncrement(bitPeriodLength / samples),
      _bitPeriodAdvance(_bitPeriodIncrement + syncBias),
      _bitPeriodRetard(_bitPeriodIncrement - syncBias),
      _bitPeriodTransition(bitPeriodLength / 2)
{

}



bool RFSync::available()
{


    // Serial.println("RFSync::RFSync");
    // Serial.printf("increment: %d\n", _bitPeriodIncrement);
    // Serial.printf("advance: %d\n", _bitPeriodAdvance);
    // Serial.printf("redard: %d\n", _bitPeriodRetard);
    // Serial.printf("transition: %d\n", _bitPeriodTransition);


    // Serial.println("RFSync::available");
    // if (!enabled())
    // {
    //     return false;
    // }

    enable();
    bool fullBuffer = _rxBufferFull;
    _rxBufferFull = false;

    // Serial.println("RFSync::available 2");
    Serial.printf("fullBuffer: %d\n", fullBuffer);
    Serial.printf("_rxBufferLen: %d\n", _rxBufferLen);

    return fullBuffer;
}

IRAM_ATTR bool RFSync::receive(uint8_t *buf, size_t *len)
{
    if (!available())
    {
        return false;
    }

    if (buf && len)
    {
        if (*len > _rxBufferLen)
        {
            *len = _rxBufferLen;
        }
        memcpy(buf, _rxBuffer, *len);
    }
    return true;
}

size_t RFSync::maxMessageLength()
{
    return _rxBufferSize;
}

void IRAM_ATTR RFSync::registerSample(bool sample)
{
    _rxCurrentSample = sample;
    if (sample)
    {
        _rxSampleCount++;
    }
}

void IRAM_ATTR RFSync::synchronize()
{
    cacheCount++;
    if (cacheCount > sizeof(lastChangeProgresses) - 1){
        cacheCount = 0;
    }
    lastChangeProgresses[cacheCount] = _rxPeriodProgress;

    if (_rxCurrentSample != _rxLastSample)
    {


        // testCount = _rxPeriodProgress;
        // probable bit transition detected

        if (_rxPeriodProgress < _bitPeriodTransition)
        {
            // we changed earlier than expected, lets
            // retard the progress counter making it
            // closer to sync with the start of the
            // current period
            _rxPeriodProgress += _bitPeriodRetard;
        }
        else
        {
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

bool IRAM_ATTR RFSync::bitTransition()
{
    return _rxPeriodProgress >= _bitPeriodLength;
}

void IRAM_ATTR RFSync::processBit()
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

    if (_rxActive)
    {


        // if we were already receiving a message

        _rxEncodedBitCount++;

        if (_rxBufferLen > 1) {
        // if (_rxEncodedBitCount == 13) {
            // testCount=_rxMsgSize;
            // disable();
        } 


        if (_rxEncodedBitCount == 12 && _rxBufferLen < _rxBufferSize)
        {
            // we have the last 12 bits of the message
            // lets decode them

            volatile uint8_t receivedByte =
                (symbol6to4(_rxLast12EncodedBits & 0x3f)) << 4 | symbol6to4(_rxLast12EncodedBits >> 6);

            // testCount = _rxLast12EncodedBits;
            // return;
            // first byte is the byte count of the incoming msg
            if (_rxBufferLen == 0)
            {
                _rxMsgSize = receivedByte;
                // testCount = _rxMsgSize;

                // testCount = _rxLast12EncodedBits;
                // disable();

                if (_rxMsgSize < 7 || _rxMsgSize > _rxBufferSize)
                {
                    // stupid message length, drop the whole thing
                    _rxActive = false;
                    return;
                }
            }


            _rxBuffer[_rxBufferLen++] = receivedByte;

            if (_rxBufferLen >= _rxMsgSize)
            {
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

        testCount = 
            (symbol6to4(_rxLast12EncodedBits & 0x3f)) << 4 | symbol6to4(_rxLast12EncodedBits >> 6);

        disable();

        // testCount = symbol6to4(_rxLast12EncodedBits >> 6);


    }
}