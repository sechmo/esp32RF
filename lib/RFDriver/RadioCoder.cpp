#include <RadioCoder.h>
#include <RFCRC.h>

// 4 bit to 6 bit symbol converter table
// Used to convert the high and low nybbles of the transmitted data
// into 6 bit symbols for transmission. Each 6-bit symbol has 3 1s and 3 0s
// with at most 3 consecutive identical bits

#define RH_DRAM_ATTR DRAM_ATTR

RH_DRAM_ATTR static uint8_t symbols[] =
    {
        0xd, 0xe, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c,
        0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34};

RadioCoder::RadioCoder(
    uint16_t speed,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t pttPin,
    uint8_t maxPayloadLen,
    uint8_t rxRampLen,
    uint8_t rampAdjust)
    : RadioSync(speed, rxPin, txPin, pttPin, maxPayloadLen, rxRampLen, rampAdjust),
      _txBuf(new uint8_t[maxPayloadLen * 2 + preambleLen]),
      maxPayloadLen(maxPayloadLen),
      maxMsgLen(maxPayloadLen - headerLen - 3)
{
    // Initialise the first 8 nibbles of the tx buffer to be the stanRCdard
    // preamble. We will append messages after that. 0x38, 0x2c is the start symbol before
    // 6-bit conversion to startSymbol
    // uint8_t preamble[preambleLen] = {0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x38, 0x2c};
    uint8_t preamble[preambleLen] = {0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, startSymbol & 0x3f, startSymbol >> 6};
    memcpy(_txBuf, preamble, sizeof(preamble));
}


uint8_t RadioCoder::maxMessageLength()
{
    return maxMsgLen;
}


// Call this often
bool RadioCoder::available()
{
    if (_mode == RHModeTx)
        return false;
    setModeRx();
    if (_rxBufFull)
    {
        validateRxBuf();
        _rxBufFull = false;
    }
    return _rxBufValid;
}

bool RH_INTERRUPT_ATTR RadioCoder::recv(uint8_t *buf, uint8_t *len)
{
    if (!available())
        return false;

    if (buf && len)
    {
        // Skip the length and 4 headers that are at the beginning of the rxBuf
        // and drop the trailing 2 bytes of FCS
        uint8_t message_len = _rxBufLen - headerLen - 3;
        if (*len > message_len)
            *len = message_len;
        memcpy(buf, _rxBuf + headerLen + 1, *len);
    }
    _rxBufValid = false; // Got the most recent message, delete it
                         //    printBuffer("recv:", buf, *len);
    return true;
}


void RH_INTERRUPT_ATTR RadioCoder::prepareTransmit()
{
    _txIndex = 0;
    _txBit = 0;
}

bool RadioCoder::waitPacketSent()
{
    while (_mode == RHModeTx)
        yield(); // Wait for any previous transmit to finish
    return true;
}

// Caution: this may block
bool RadioCoder::send(const uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint16_t index = 0;
    uint16_t crc = 0xffff;
    uint8_t *p = _txBuf + preambleLen;   // start of the message area
    uint8_t count = len + 3 + headerLen; // Added byte count and FCS and headers to get total number of bytes

    if (len > maxMsgLen)
        return false;

    // Wait for transmitter to become available
    waitPacketSent();

    // Encode the message length
    crc = RHcrc_ccitt_update(crc, count);
    p[index++] = symbols[count >> 4];
    p[index++] = symbols[count & 0xf];

    // Encode the headers
    crc = RHcrc_ccitt_update(crc, _txHeaderTo);
    p[index++] = symbols[_txHeaderTo >> 4];
    p[index++] = symbols[_txHeaderTo & 0xf];
    crc = RHcrc_ccitt_update(crc, _txHeaderFrom);
    p[index++] = symbols[_txHeaderFrom >> 4];
    p[index++] = symbols[_txHeaderFrom & 0xf];
    crc = RHcrc_ccitt_update(crc, _txHeaderId);
    p[index++] = symbols[_txHeaderId >> 4];
    p[index++] = symbols[_txHeaderId & 0xf];
    crc = RHcrc_ccitt_update(crc, _txHeaderFlags);
    p[index++] = symbols[_txHeaderFlags >> 4];
    p[index++] = symbols[_txHeaderFlags & 0xf];

    // Encode the message into 6 bit symbols. Each byte is converted into
    // 2 6-bit symbols, high nybble first, low nybble second
    for (i = 0; i < len; i++)
    {
        crc = RHcrc_ccitt_update(crc, data[i]);
        p[index++] = symbols[data[i] >> 4];
        p[index++] = symbols[data[i] & 0xf];
    }

    // Append the fcs, 16 bits before encoding (4 6-bit symbols after encoding)
    // Caution: VW expects the _ones_complement_ of the CCITT CRC-16 as the FCS
    // VW sends FCS as low byte then hi byte
    crc = ~crc;
    p[index++] = symbols[(crc >> 4) & 0xf];
    p[index++] = symbols[crc & 0xf];
    p[index++] = symbols[(crc >> 12) & 0xf];
    p[index++] = symbols[(crc >> 8) & 0xf];

    // Total number of 6-bit symbols to send
    _txBufLen = index + preambleLen;

    // Start the low level interrupt handler sending symbols
    setModeTx();

    return true;
}



// Convert a 6 bit encoded symbol into its 4 bit decoded equivalent
uint8_t RH_INTERRUPT_ATTR RadioCoder::symbol_6to4(uint8_t symbol)
{
    uint8_t i;
    uint8_t count;

    // Linear search :-( Could have a 64 byte reverse lookup table?
    // There is a little speedup here courtesy Ralph Doncaster:
    // The shortcut works because bit 5 of the symbol is 1 for the last 8
    // symbols, and it is 0 for the first 8.
    // So we only have to search half the table
    for (i = (symbol >> 2) & 8, count = 8; count--; i++)
        if (symbol == symbols[i])
            return i;

    return 0; // Not found
}


uint8_t RadioCoder::decodeByte(uint16_t receivedBits)
{
    // Decode the 12 bits into 2 lots of 6 bits
    // The 6 lsbits are the high nybble
    return (symbol_6to4(receivedBits & 0x3f) << 4) | symbol_6to4(receivedBits >> 6);
}

bool RH_INTERRUPT_ATTR RadioCoder::detectedMsgStart(uint16_t receivedBits)
{
    return receivedBits == startSymbol;
}


// Send next bit
// Symbols are sent LSB first
// Finished sending the whole message? (after waiting one bit period
// since the last bit)
bool RH_INTERRUPT_ATTR RadioCoder::moreBitsToTransmit() {
    return _txIndex < _txBufLen;
}


bool RH_INTERRUPT_ATTR RadioCoder::nextBitToTransmit() {

    bool nextBit = _txBuf[_txIndex] & (1 << _txBit);

    _txBit++;

    if (_txBit >= 6)
    {
        _txBit = 0;
        _txIndex++;
    }


    return nextBit;
}

