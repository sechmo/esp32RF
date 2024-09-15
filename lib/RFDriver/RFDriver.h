// RH_ASK.h
//
// Copyright (C) 2014 Mike McCauley
// $Id: RH_ASK.h,v 1.22 2020/05/06 22:26:45 mikem Exp $

#ifndef RH_ASK_h
#define RH_ASK_h

#include <Arduino.h>

#define RH_INTERRUPT_ATTR IRAM_ATTR
// #include <RFGenericDriver.h>


class RadioDriver //: public RHGenericDriver
{
public:
    typedef enum
    {
        RHModeInitialising = 0, ///< Transport is initialising. Initial default value until init() is called..
        RHModeSleep,            ///< Transport hardware is in low power sleep mode (if supported)
        RHModeIdle,             ///< Transport is idle.
        RHModeTx,               ///< Transport is in the process of transmitting a message.
        RHModeRx,               ///< Transport is in the process of receiving a message.
        RHModeCad               ///< Transport is in the process of detecting channel activity (if supported)
    } RHMode;

    /// Constructor.
    /// At present only one instance of RH_ASK per sketch is supported.
    /// \param[in] speed The desired bit rate in bits per second
    /// \param[in] rxPin The pin that is used to get data from the receiver
    /// \param[in] txPin The pin that is used to send data to the transmitter
    /// \param[in] pttPin The pin that is connected to the transmitter controller. It will be set HIGH to enable the transmitter (unless pttInverted is true).
    /// \param[in] pttInverted true if you desire the pttin to be inverted so that LOW wil enable the transmitter.
    RadioDriver(
        uint16_t speed = 2000,
        uint8_t rxPin = 11,
        uint8_t txPin = 12,
        uint8_t pttPin = 10,
        uint8_t maxPayloadLen = 67
        );


    /// Initialise the Driver transport hardware and software.
    /// Make sure the Driver is properly configured before calling init().
    /// \return true if initialisation succeeded.
    virtual bool init();


    /// If current mode is Rx or Tx changes it to Idle. If the transmitter or receiver is running,
    /// disables them.
    RH_INTERRUPT_ATTR void setModeIdle();

    /// If current mode is Tx or Idle, changes it to Rx.
    /// Starts the receiver in the RF69.
    RH_INTERRUPT_ATTR void setModeRx();

    /// If current mode is Rx or Idle, changes it to Rx. F
    /// Starts the transmitter in the RF69.
    void setModeTx();

    /// dont call this it used by the interrupt handler
    void RH_INTERRUPT_ATTR handleTimerInterrupt();

    /// Returns the current speed in bits per second
    /// \return The current speed in bits per second
    uint16_t speed() { return _speed; }

protected:

    const uint8_t rxSamples = 8;

    volatile uint16_t _txGood;

    volatile RHMode _mode;

    /// Set up the timer and its interrutps so the interrupt handler is called at the right frequency
    void timerSetup();

    /// Read the rxPin in a platform dependent way, taking into account whether it is inverted or not
    RH_INTERRUPT_ATTR bool readRx();

    /// Write the txPin in a platform dependent way
    void writeTx(bool value);

    /// Write the txPin in a platform dependent way, taking into account whether it is inverted or not
    void writePtt(bool value);

    /// The receiver handler function, called a 8 times the bit rate
    void receiveTimer();

    virtual void registerSample(bool rxSample) = 0;
    virtual void synchronize() = 0;
    virtual bool bitTransition() = 0;
    virtual void processBit() = 0;

    /// The transmitter handler function, called a 8 times the bit rate
    void transmitTimer();

    virtual void prepareTransmit() = 0;
    virtual bool moreBitsToTransmit() = 0;
    virtual bool nextBitToTransmit() = 0;

    /// Configure bit rate in bits per second
    uint16_t _speed;

    /// The configure receiver pin
    uint8_t _rxPin;

    /// The configure transmitter pin
    uint8_t _txPin;

    /// The configured transmitter enable pin
    uint8_t _pttPin;

    /// Sample number for the transmitter. Runs 0 to 7 during one bit interval
    uint8_t _txSample;

};


/// @example ask_reliable_datagram_client.ino
/// @example ask_reliable_datagram_server.ino
/// @example ask_transmitter.ino
/// @example ask_receiver.ino
#endif
