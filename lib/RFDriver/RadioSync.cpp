#include <RadioSync.h>
#include <RFCRC.h>

RadioSync::RadioSync(
    uint16_t speed,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t pttPin,
    uint8_t maxPayloadLen,
    uint8_t rxSamples,
    uint8_t rxRampLen,
    uint8_t rampAdjust)
    : RadioDriver(speed, rxPin, txPin, pttPin, maxPayloadLen, rxSamples, rxRampLen, rampAdjust),
      _rxIntegrator(0),
      rxRampLen(rxRampLen),
      rampTransition(rxRampLen / 2),
      _rxBad(0),
      _rxGood(0),
      _rxBufValid(false),
      _rxBufFull(false),
      _rxBufLen(0),
      _rxBuf(new uint8_t[maxPayloadLen]),
      rampInc(rxRampLen / rxSamples),
      rampAdjust(rampAdjust),
      rampIncRetard(rampInc - rampAdjust),
      rampIncAdvance(rampInc + rampAdjust)
{
}

// Call this often
bool RadioSync::available()
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

bool RH_INTERRUPT_ATTR RadioSync::recv(uint8_t *buf, uint8_t *len)
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


// Check whether the latest received message is complete and uncorrupted
// We should always check the FCS at user level, not interrupt level
// since it is slow
void RadioSync::validateRxBuf()
{
    uint16_t crc = 0xffff;
    // The CRC covers the byte count, headers and user data
    for (uint8_t i = 0; i < _rxBufLen; i++)
        crc = RHcrc_ccitt_update(crc, _rxBuf[i]);
    if (crc != 0xf0b8) // CRC when buffer and expected CRC are CRC'd
    {
        // Reject and drop the message
        _rxBad++;
        _rxBufValid = false;
        return;
    }

    // Extract the 4 headers that follow the message length
    _rxHeaderTo = _rxBuf[1];
    _rxHeaderFrom = _rxBuf[2];
    _rxHeaderId = _rxBuf[3];
    _rxHeaderFlags = _rxBuf[4];

    _rxGood++;
    _rxBufValid = true;
}

void RH_INTERRUPT_ATTR RadioSync::registerSample(bool rxSample)
{

    _rxCurrentSample = rxSample;
    // Integrate each sample
    if (rxSample)
        _rxIntegrator++;
}

void RH_INTERRUPT_ATTR RadioSync::synchronize()
{
    if (_rxCurrentSample != _rxLastSample)
    {
        // Transition, advance if ramp > 80, retard if < 80
        _rxPllRamp += ((_rxPllRamp < rampTransition)
                           ? rampIncRetard
                           : rampIncAdvance);
        _rxLastSample = _rxCurrentSample;
    }
    else
    {
        // No transition
        // Advance ramp by standard 20 (== 160/8 samples)
        _rxPllRamp += rampInc;
    }
}

bool RH_INTERRUPT_ATTR RadioSync::bitTransition()
{
    return _rxPllRamp >= rxRampLen;
}

void RH_INTERRUPT_ATTR RadioSync::processBit()
{

    // Add this to the 12th bit of _rxBits, LSB first
    // The last 12 bits are kept
    _rxBits >>= 1;

    // Check the integrator to see how many samples in this cycle were high.
    // If < 5 out of 8, then its declared a 0 bit, else a 1;
    if (_rxIntegrator >= 5)
        _rxBits |= 0x800;

    _rxPllRamp -= rxRampLen;
    _rxIntegrator = 0; // Clear the integral for the next cycle

    if (_rxActive)
    {
        // We have the start symbol and now we are collecting message bits,
        // 6 per symbol, each which has to be decoded to 4 bits
        if (++_rxBitCount >= 12)
        {
            // Have 12 bits of encoded message == 1 byte encoded
            // Decode as 2 lots of 6 bits into 2 lots of 4 bits
            // The 6 lsbits are the high nybble
            uint8_t this_byte =
                (symbol_6to4(_rxBits & 0x3f)) << 4 | symbol_6to4(_rxBits >> 6);

            // The first decoded byte is the byte count of the following message
            // the count includes the byte count and the 2 trailing FCS bytes
            // REVISIT: may also include the ACK flag at 0x40
            if (_rxBufLen == 0)
            {
                // The first byte is the byte count
                // Check it for sensibility. It cant be less than 7, since it
                // includes the byte count itself, the 4 byte header and the 2 byte FCS
                _rxCount = this_byte;

                // setModeIdle();
                // return;
                if (_rxCount < 7 || _rxCount > maxPayloadLen)
                {
                    // Stupid message length, drop the whole thing
                    _rxActive = false;
                    _rxBad++;
                    return;
                }
            }
            _rxBuf[_rxBufLen++] = this_byte;

            if (_rxBufLen >= _rxCount)
            {
                // Got all the bytes now
                _rxActive = false;
                _rxBufFull = true;
                setModeIdle();
            }
            _rxBitCount = 0;
        }
    }
    // Not in a message, see if we have a start symbol
    else if (_rxBits == startSymbol)
    {
        // Have start symbol, start collecting message
        _rxActive = true;
        _rxBitCount = 0;
        _rxBufLen = 0;

        setModeIdle();
    }
}
