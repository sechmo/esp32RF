#include <RadioSync.h>
#include <RFCRC.h>

RadioSync::RadioSync(
    uint16_t speed,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t pttPin,
    uint8_t maxPayloadLen,
    uint8_t rxRampLen,
    uint8_t rampAdjust)
    : RadioDriver(speed, rxPin, txPin, pttPin, maxPayloadLen),
      _rxIntegrator(0),
      rxRampLen(rxRampLen),
      rampTransition(rxRampLen / 2),
      _rxBad(0),
      _rxGood(0),
      _rxBufFull(false),
      _rxBufLen(0),
      _rxBuf(new uint8_t[maxPayloadLen]),
      rampInc(rxRampLen / rxSamples),
      rampAdjust(rampAdjust),
      rampIncRetard(rampInc - rampAdjust),
      rampIncAdvance(rampInc + rampAdjust)
{
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

    _rxPllRamp -= rxRampLen;
    // Add this to the 12th bit of _rxBits, LSB first
    // The last 12 bits are kept

    bool bit = _rxIntegrator >= 5;
    _rxIntegrator = 0; // Clear the integral for the next cycle


    receiveBit(bit);

}

void RH_INTERRUPT_ATTR RadioSync::receiveBit(bool bit)
{
    _rxBits >>= 1;
    // Check the integrator to see how many samples in this cycle were high.
    // If < 5 out of 8, then its declared a 0 bit, else a 1;
    if (bit)
        _rxBits |= 0x800;


    if (_rxActive)
    {

        // We have the start symbol and now we are collecting message bits,
        // 6 per symbol, each which has to be decoded to 4 bits
        if (++_rxBitCount >= 12)
        {
            // Have 12 bits of encoded message == 1 byte encoded
            // Decode as 2 lots of 6 bits into 2 lots of 4 bits
            // The 6 lsbits are the high nybble
            uint8_t this_byte = decodeByte(_rxBits);
                // (symbol_6to4(_rxBits & 0x3f)) << 4 | symbol_6to4(_rxBits >> 6);

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
                // if (_rxCount < 7 || _rxCount > maxPayloadLen)
                // {
                //     // Stupid message length, drop the whole thing
                //     _rxActive = false;
                //     _rxBad++;
                //     return;
                // }
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
    else if (detectedMsgStart(_rxBits))
    {
        // Have start symbol, start collecting message
        _rxActive = true;
        _rxBitCount = 0;
        _rxBufLen = 0;

        setModeIdle();
    }
}

