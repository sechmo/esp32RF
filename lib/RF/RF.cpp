#include <RF.h>

RFReceiver* RFReceiver::instance = nullptr;

RFReceiver::RFReceiver(uint16_t speed, uint8_t rxPin)
    : _speed(speed), _rxPin(rxPin)
{
    disable();
}

bool RFReceiver::init()
{
    pinMode(_rxPin, INPUT);
    enable();
    timerSetup();

    return true;
}

// base timer frequency is 80MHz, if we divide by 80 we get 1MHz
// if we use this divider the timer will tick each microsecond
const int divider = 80;
const int microsecPerSec = 1000000;


void RFReceiver::timerSetup()
{
    const int ticksPerBit = microsecPerSec / _speed;
    const int ticksPerSample = ticksPerBit / samples;


    timer = timerBegin(0, divider, true);
    timerAttachInterrupt(timer, &handleTimerInterruptStatic, true);
    timerAlarmWrite(timer, ticksPerSample, true);
}

IRAM_ATTR bool RFReceiver::readRx()
{
    return digitalRead(_rxPin);
}

IRAM_ATTR void RFReceiver::handleTimerInterrupt()
{
    if (_enabled) {
        receiveTimer();
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