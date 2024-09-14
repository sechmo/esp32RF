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