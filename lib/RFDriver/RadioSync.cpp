#include <RadioSync.h>


RadioSync::RadioSync(
    uint16_t speed,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t pttPin,
    uint8_t maxPayloadLen,
    uint8_t rxSamples,
    uint8_t rxRampLen,
    uint8_t rampAdjust
    )
    : RadioDriver(speed, rxPin, txPin, pttPin, maxPayloadLen, rxSamples, rxRampLen, rampAdjust)
{
}
