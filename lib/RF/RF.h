#include <Arduino.h>
const int MAX_MESSAGE_LEN = 30;
// number of samples per bit
const uint16_t samples = 8;

class RFReceiver
{
private: 

    static RFReceiver *instance;

    IRAM_ATTR hw_timer_t *timer = nullptr;

public:

    RFReceiver(uint16_t speed = 2000, uint8_t rxPin = 11);

    // Initialise the driver and the transport hardware.
    bool init();

    // True when a new message is available
    bool available();

    
    // Ram loaded function that will copy the message to the buffer
    IRAM_ATTR virtual bool receive(uint8_t* buf, size_t len);


    size_t maxMessageLength();


    uint16_t speed() { return _speed; }

    bool enabled() { return _enabled; }

    void disable() { _enabled = false; };
    void enable() { _enabled = true; }

    static void IRAM_ATTR handleTimerInterruptStatic()
    {
        if (instance != nullptr)
        {
            instance->handleTimerInterrupt();
        }
    }

protected:

    virtual void registerSample(bool sample) = 0;
    virtual void synchronize() = 0;
    virtual bool bitTransition() = 0;
    virtual void processBit() = 0;

    // Setup timer and interrupts
    void timerSetup();

    // Read the rxPin
    IRAM_ATTR bool readRx();

    // function run by the timer interrupt
    IRAM_ATTR void receiveTimer();

    // handle the timer interrupt if is  enabled
    IRAM_ATTR void handleTimerInterrupt();


    uint16_t _speed;
    uint8_t _rxPin;

    bool _enabled = false;
};