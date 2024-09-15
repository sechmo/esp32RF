// RH_ASK.cpp
//
// Copyright (C) 2014 Mike McCauley
// $Id: RH_ASK.cpp,v 1.32 2020/08/04 09:02:14 mikem Exp $

#include <RFDriver.h>



// Michael Cain
DRAM_ATTR hw_timer_t *timer;
// jPerotto Non-constant static data from ESP32 https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/general-notes.html#dram-data-ram

// RH_ASK on Arduino uses Timer 1 to generate interrupts 8 times per bit interval
// Define RH_ASK_ARDUINO_USE_TIMER2 if you want to use Timer 2 instead of Timer 1 on Arduino
// You may need this to work around other libraries that insist on using timer 1
// Should be moved to header file
// #define RH_ASK_ARDUINO_USE_TIMER2

// RH_ASK on ATtiny8x uses Timer 0 to generate interrupts 8 times per bit interval.
// Timer 0 is used by Arduino platform for millis()/micros() which is used by delay()
// Uncomment the define RH_ASK_ATTINY_USE_TIMER1 bellow, if you want tRC

// Interrupt handler uses this to find the most recently initialised instance of this driver
static RadioDriver *thisASKDriver;

// This is the value of the start symbol after 6-bit conversion and nybble swapping
// #define startSymbol 0xb38

RadioDriver::RadioDriver(
    uint16_t speed,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t pttPin,
    uint8_t maxPayloadLen)
    : _speed(speed),
      _rxPin(rxPin),
      _txPin(txPin),
      _pttPin(pttPin),
      _mode(RHModeInitialising),
      _txGood(0),
      maxPayloadLen(maxPayloadLen),
      maxMsgLen(maxPayloadLen - headerLen - 3)
{
}

bool RadioDriver::init()
{
    thisASKDriver = this;

    // Set up digital IO pins for arduino
    pinMode(_txPin, OUTPUT);
    pinMode(_rxPin, INPUT);
    pinMode(_pttPin, OUTPUT);


    // Serial.print("start symbol");
    // Serial.println(startSymbol, HEX);
    // Serial.print("start symbol 6to4 lower");
    // Serial.println(symbols[startSymbol & 0x3f], HEX);
    // Serial.print("start symbol 6to4 upper");
    // Serial.println(symbols[startSymbol >> 6], HEX);


    // Ready to go
    setModeIdle();
    timerSetup();

    return true;
}

// The idea here is to get 8 timer interrupts per bit period
void RadioDriver::timerSetup()
{
    void RH_INTERRUPT_ATTR esp32_timer_interrupt_handler(); // Forward declaration
    // Prior to version 3
    timer = timerBegin(0, 80, true); // Alarm value will be in in us
    timerAttachInterrupt(timer, &esp32_timer_interrupt_handler, true);
    timerAlarmWrite(timer, 1000000 / _speed / rxSamples, true);
    timerAlarmEnable(timer);
}

void RH_INTERRUPT_ATTR RadioDriver::setModeIdle()
{
    if (_mode != RHModeIdle)
    {
        // Disable the transmitter hardware
        writePtt(LOW);
        writeTx(LOW);
        _mode = RHModeIdle;
    }
}

void RH_INTERRUPT_ATTR RadioDriver::setModeRx()
{
    if (_mode != RHModeRx)
    {
        // Disable the transmitter hardware
        writePtt(LOW);
        writeTx(LOW);
        _mode = RHModeRx;
    }
}

void RadioDriver::setModeTx()
{
    if (_mode != RHModeTx)
    {
        prepareTransmit();
        _txSample = 0;

        // Enable the transmitter hardware
        writePtt(HIGH);

        _mode = RHModeTx;
    }
}


// Read the RX data input pin, taking into account platform type and inversion.
bool RH_INTERRUPT_ATTR RadioDriver::readRx()
{
    bool value;
    value = digitalRead(_rxPin);
    return value;
}

// Write the TX output pin, taking into account platform type.
void RH_INTERRUPT_ATTR RadioDriver::writeTx(bool value)
{
    digitalWrite(_txPin, value);
}

// Write the PTT output pin, taking into account platform type and inversion.
void RH_INTERRUPT_ATTR RadioDriver::writePtt(bool value)
{
    digitalWrite(_pttPin, value);
}

uint8_t RadioDriver::maxMessageLength()
{
    return maxMsgLen;
}

void RH_INTERRUPT_ATTR esp32_timer_interrupt_handler()
{
    thisASKDriver->handleTimerInterrupt();
}


void RH_INTERRUPT_ATTR RadioDriver::receiveTimer()
{
    bool rxSample = readRx();

    registerSample(rxSample);

    synchronize();
    if (bitTransition())
    {
        processBit();
    }
}

void RH_INTERRUPT_ATTR RadioDriver::transmitTimer()
{
    if (_txSample++ == 0)
    {
        // Send next bit
        // Symbols are sent LSB first
        // Finished sending the whole message? (after waiting one bit period
        // since the last bit)
        if (moreBitsToTransmit()) {
            writeTx(nextBitToTransmit());
        }
        else {
            setModeIdle();
            _txGood++;
        }
    }

    if (_txSample >= rxSamples) 
        _txSample = 0;
}
void RH_INTERRUPT_ATTR RadioDriver::handleTimerInterrupt()
{
    if (_mode == RHModeRx)
        receiveTimer(); // Receiving
    else if (_mode == RHModeTx)
        transmitTimer(); // Transmitting
}
