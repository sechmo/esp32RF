#include <RFDriver.h>

class RadioSync : public RadioDriver
{
public:
    RadioSync(
        uint16_t speed = 2000,
        uint8_t rxPin = 11,
        uint8_t txPin = 12,
        uint8_t pttPin = 10,
        uint8_t maxPayloadLen = 67,
        uint8_t rxRampLen = 160,
        uint8_t rampAdjust = 9);

protected:
    void validateRxBuf();

    void registerSample(bool rxSample) override;

    void synchronize() override;
    bool bitTransition() override;
    void processBit() override;

    virtual uint8_t decodeByte(uint16_t receivedBits) = 0;

    /// TO header in the last received mesasge
    volatile uint8_t _rxHeaderTo;

    /// FROM header in the last received mesasge
    volatile uint8_t _rxHeaderFrom;

    /// ID header in the last received mesasge
    volatile uint8_t _rxHeaderId;

    /// FLAGS header in the last received mesasge
    volatile uint8_t _rxHeaderFlags;

    // Used in the interrupt handlers
    /// Buf is filled but not validated
    volatile bool _rxBufFull;

    /// Buf is full and valid
    volatile bool _rxBufValid;

    /// The incoming message buffer
    uint8_t *_rxBuf;

    /// The incoming message expected length
    volatile uint8_t _rxCount;

    /// The incoming message buffer length received so far
    volatile uint8_t _rxBufLen;

    /// The size of the receiver ramp. Ramp wraps modulo this number
    const uint8_t rxRampLen = 160;

    // Ramp adjustment parameters
    // Standard is if a transition occurs before rampTransition (80) in the ramp,
    // the ramp is retarded by adding rampIncRetard (11)
    // else by adding rampIncAdvance (29)
    // If there is no transition it is adjusted by rampInc (20)
    /// Internal ramp adjustment parameter
    const uint8_t rampInc;
    const uint8_t rampTransition;
    const uint8_t rampAdjust;
    const uint8_t rampIncRetard;
    const uint8_t rampIncAdvance;

    volatile bool _rxCurrentSample;
    /// Last digital input from the rx data pin
    volatile bool _rxLastSample;

    /// This is the integrate and dump integral. If there are <5 0 samples in the PLL cycle
    /// the bit is declared a 0, else a 1
    volatile uint8_t _rxIntegrator;

    /// PLL ramp, varies between 0 and rxRampLen-1 (159) over
    /// rxSamples (8) samples per nominal bit time.
    /// When the PLL is synchronised, bit transitions happen at about the
    /// 0 mark.
    volatile uint8_t _rxPllRamp;

    /// Flag indicates if we have seen the start symbol of a new message and are
    /// in the processes of reading and decoding it
    volatile uint8_t _rxActive;

    /// Last 12 bits received, so we can look for the start symbol
    volatile uint16_t _rxBits;

    /// How many bits of message we have received. Ranges from 0 to 12
    volatile uint8_t _rxBitCount;

    volatile uint16_t _rxBad;
    volatile uint16_t _rxGood;
};