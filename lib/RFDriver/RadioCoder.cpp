#include <RadioCoder.h>


RadioCoder::RadioCoder(
    uint16_t speed,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t pttPin,
    uint8_t maxPayloadLen,
    uint8_t rxRampLen,
    uint8_t rampAdjust)
    : RadioSync(speed, rxPin, txPin, pttPin, maxPayloadLen, rxRampLen, rampAdjust)
{
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