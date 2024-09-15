// RH_ASK.cpp
//
// Copyright (C) 2014 Mike McCauley
// $Id: RH_ASK.cpp,v 1.32 2020/08/04 09:02:14 mikem Exp $

#include <RFDriver.h>
#include <RFCRC.h>

bool RadioDriver::waitPacketSent()
{
    while (_mode == RHModeTx)
        yield(); // Wait for any previous transmit to finish
    return true;
}

bool RadioDriver::waitCAD()
{
    if (!_cad_timeout)
        return true;

    // Wait for any channel activity to finish or timeout
    // Sophisticated DCF function...
    // DCF : BackoffTime = random() x aSlotTime
    // 100 - 1000 ms
    // 10 sec timeout
    unsigned long t = millis();
    while (isChannelActive())
    {
        if (millis() - t > _cad_timeout)
            return false;
        delay(random(1, 10) * 100); // Should these values be configurable? Macros?
    }

    return true;
}

// subclasses are expected to override if CAD is available for that radio
bool RadioDriver::isChannelActive()
{
    return false;
}

// Michael Cain
DRAM_ATTR hw_timer_t *timer;
// jPerotto Non-constant static data from ESP32 https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/general-notes.html#dram-data-ram
#define RH_DRAM_ATTR DRAM_ATTR

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

// 4 bit to 6 bit symbol converter table
// Used to convert the high and low nybbles of the transmitted data
// into 6 bit symbols for transmission. Each 6-bit symbol has 3 1s and 3 0s
// with at most 3 consecutive identical bits
RH_DRAM_ATTR static uint8_t symbols[] =
    {
        0xd, 0xe, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c,
        0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34};

// This is the value of the start symbol after 6-bit conversion and nybble swapping
// #define startSymbol 0xb38

RadioDriver::RadioDriver(
    uint16_t speed,
    uint8_t rxPin,
    uint8_t txPin,
    uint8_t pttPin,
    uint8_t maxPayloadLen,
    uint8_t rxSamples,
    uint8_t rxRampLen,
    uint8_t rampAdjust)
    : _speed(speed),
      _rxPin(rxPin),
      _txPin(txPin),
      _pttPin(pttPin),
      _cad_timeout(0),
      _mode(RHModeInitialising),
      _txGood(0),
      rxSamples(rxSamples),
      maxPayloadLen(maxPayloadLen),
      maxMsgLen(maxPayloadLen - headerLen - 3),
      _txBuf(new uint8_t[maxPayloadLen * 2 + preambleLen])
{
    // Initialise the first 8 nibbles of the tx buffer to be the stanRCdard
    // preamble. We will append messages after that. 0x38, 0x2c is the start symbol before
    // 6-bit conversion to startSymbol
    // uint8_t preamble[preambleLen] = {0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x38, 0x2c};
    uint8_t preamble[preambleLen] = {0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, symbol_6to4(startSymbol & 0x3f), symbol_6to4(startSymbol >> 6)};
    memcpy(_txBuf, preamble, sizeof(preamble));
}

bool RadioDriver::init()
{
    thisASKDriver = this;

    // Set up digital IO pins for arduino
    pinMode(_txPin, OUTPUT);
    pinMode(_rxPin, INPUT);
    pinMode(_pttPin, OUTPUT);

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
        // PRepare state varibles for a new transmission
        _txIndex = 0;
        _txBit = 0;
        _txSample = 0;

        // Enable the transmitter hardware
        writePtt(HIGH);

        _mode = RHModeTx;
    }
}

// Caution: this may block
bool RadioDriver::send(const uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint16_t index = 0;
    uint16_t crc = 0xffff;
    uint8_t *p = _txBuf + preambleLen;   // start of the message area
    uint8_t count = len + 3 + headerLen; // Added byte count and FCS and headers to get total number of bytes

    if (len > maxMsgLen)
        return false;

    // Wait for transmitter to become available
    waitPacketSent();

    if (!waitCAD())
        return false; // Check channel activity

    // Encode the message length
    crc = RHcrc_ccitt_update(crc, count);
    p[index++] = symbols[count >> 4];
    p[index++] = symbols[count & 0xf];

    // Encode the headers
    crc = RHcrc_ccitt_update(crc, _txHeaderTo);
    p[index++] = symbols[_txHeaderTo >> 4];
    p[index++] = symbols[_txHeaderTo & 0xf];
    crc = RHcrc_ccitt_update(crc, _txHeaderFrom);
    p[index++] = symbols[_txHeaderFrom >> 4];
    p[index++] = symbols[_txHeaderFrom & 0xf];
    crc = RHcrc_ccitt_update(crc, _txHeaderId);
    p[index++] = symbols[_txHeaderId >> 4];
    p[index++] = symbols[_txHeaderId & 0xf];
    crc = RHcrc_ccitt_update(crc, _txHeaderFlags);
    p[index++] = symbols[_txHeaderFlags >> 4];
    p[index++] = symbols[_txHeaderFlags & 0xf];

    // Encode the message into 6 bit symbols. Each byte is converted into
    // 2 6-bit symbols, high nybble first, low nybble second
    for (i = 0; i < len; i++)
    {
        crc = RHcrc_ccitt_update(crc, data[i]);
        p[index++] = symbols[data[i] >> 4];
        p[index++] = symbols[data[i] & 0xf];
    }

    // Append the fcs, 16 bits before encoding (4 6-bit symbols after encoding)
    // Caution: VW expects the _ones_complement_ of the CCITT CRC-16 as the FCS
    // VW sends FCS as low byte then hi byte
    crc = ~crc;
    p[index++] = symbols[(crc >> 4) & 0xf];
    p[index++] = symbols[crc & 0xf];
    p[index++] = symbols[(crc >> 12) & 0xf];
    p[index++] = symbols[(crc >> 8) & 0xf];

    // Total number of 6-bit symbols to send
    _txBufLen = index + preambleLen;

    // Start the low level interrupt handler sending symbols
    setModeTx();

    return true;
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

// Convert a 6 bit encoded symbol into its 4 bit decoded equivalent
uint8_t RH_INTERRUPT_ATTR RadioDriver::symbol_6to4(uint8_t symbol)
{
    uint8_t i;
    uint8_t count;

    // Linear search :-( Could have a 64 byte reverse lookup table?
    // There is a little speedup here courtesy Ralph Doncaster:
    // The shortcut works because bit 5 of the symbol is 1 for the last 8
    // symbols, and it is 0 for the first 8.
    // So we only have to search half the table
    for (i = (symbol >> 2) & 8, count = 8; count--; i++)
        if (symbol == symbols[i])
            return i;

    return 0; // Not found
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
        if (_txIndex >= _txBufLen)
        {
            setModeIdle();
            _txGood++;
        }
        else
        {
            writeTx(_txBuf[_txIndex] & (1 << _txBit++));
            if (_txBit >= 6)
            {
                _txBit = 0;
                _txIndex++;
            }
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
