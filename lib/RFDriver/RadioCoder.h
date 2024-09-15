#include <RadioSync.h>


class RadioCoder: public RadioSync
{
public:
    RadioCoder(
        uint16_t speed = 2000,
        uint8_t rxPin = 11,
        uint8_t txPin = 12,
        uint8_t pttPin = 10,
        uint8_t maxPayloadLen = 67,
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
};