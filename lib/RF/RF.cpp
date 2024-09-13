#include <RF.h>

RFReceiver* RFReceiver::instance = nullptr;

static RFReceiver* thisReceiver;

RFReceiver::RFReceiver(uint16_t speed, uint8_t rxPin, uint16_t samples)
    : _speed(speed), _rxPin(rxPin), _samples(samples)
{
    disable();
    instance = this;
    thisReceiver = this;
}

bool RFReceiver::init()
{

    Serial.println("RFReceiver::init");
    pinMode(_rxPin, INPUT);
    enable();
    timerSetup();

    return true;
}

// base timer frequency is 80MHz, if we divide by 80 we get 1MHz
// if we use this divider the timer will tick each microsecond
const int divider = 80;
const int microsecPerSec = 1000000;


void test()
{
    thisReceiver->testCount++;
}

void RFReceiver::timerSetup()
{
    const int ticksPerBit = microsecPerSec / _speed;
    const int ticksPerSample = ticksPerBit / _samples;

    // const int ticksPerBit = microsecPerSec / 1;
    // const int ticksPerSample = ticksPerBit / 1;

    Serial.println("RFReceiver::timerSetup");
    Serial.printf("ticksPerBit: %d\n", ticksPerBit);
    Serial.printf("ticksPerSample: %d\n", ticksPerSample);


    timer = timerBegin(0, divider, true);
    timerAttachInterrupt(timer, &handleTimerInterruptStatic, true);
    // timerAttachInterrupt(timer, &test, true);
    timerAlarmWrite(timer, ticksPerSample, true);
    timerAlarmEnable(timer);
}

IRAM_ATTR bool RFReceiver::readRx()
{
    bool value;

    value = digitalRead(_rxPin);

    return value;
}

IRAM_ATTR void RFReceiver::handleTimerInterrupt()
{
    if (_enabled) {
        receiveTimer();
    }
}



void IRAM_ATTR RFReceiver::handleTimerInterruptStatic()
{
    if (instance)
    {
        instance->handleTimerInterrupt();
    }
}

IRAM_ATTR void RFReceiver::receiveTimer()
{
    bool sample = readRx();

    registerSample(sample);

    synchronize();

    if (bitTransition()) {
       processBit();
    }
}