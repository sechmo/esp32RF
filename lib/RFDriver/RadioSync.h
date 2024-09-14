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
        uint8_t rxSamples = 8,
        uint8_t rxRampLen = 160,
        uint8_t rampAdjust = 9);



    /// Tests whether a new message is available
    /// from the Driver.
    /// On most drivers, this will also put the Driver into RHModeRx mode until
    /// a message is actually received bythe transport, when it wil be returned to RHModeIdle.
    /// This can be called multiple times in a timeout loop
    /// \return true if a new, complete, error-free uncollected message is available to be retreived by recv()
    virtual bool available();


    /// Turns the receiver on if it not already on.
    /// If there is a valid message available, copy it to buf and return true
    /// else return false.
    /// If a message is copied, *len is set to the length (Caution, 0 length messages are permitted).
    /// You should be sure to call this function frequently enough to not miss any messages
    /// It is recommended that you call it in your main loop.
    /// \param[in] buf Location to copy the received message
    /// \param[in,out] len Pointer to the number of octets available in buf. The number be reset to the actual number of octets copied.
    /// \return true if a valid message was copied to buf
    RH_INTERRUPT_ATTR virtual bool recv(uint8_t *buf, uint8_t *len);


protected:
    void validateRxBuf();

    void registerSample(bool rxSample) override;

    void synchronize() override;
    bool bitTransition() override;
    void processBit() override;



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