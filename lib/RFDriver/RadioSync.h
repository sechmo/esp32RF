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

protected:

    void registerSample(bool rxSample) override;

    void synchronize() override;
};