// RH_ASK.h
//
// Copyright (C) 2014 Mike McCauley
// $Id: RH_ASK.h,v 1.22 2020/05/06 22:26:45 mikem Exp $

#ifndef RH_ASK_h
#define RH_ASK_h

#include <Arduino.h>

#define RH_INTERRUPT_ATTR IRAM_ATTR
// #include <RFGenericDriver.h>


class RH_ASK //: public RHGenericDriver
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
    RH_ASK(
        uint16_t speed = 2000,
        uint8_t rxPin = 11,
        uint8_t txPin = 12,
        uint8_t pttPin = 10,
        uint8_t maxPayloadLen = 67,
        uint8_t rxSamples = 8, 
        uint8_t rxRampLen = 160,
        uint8_t rampAdjust = 9
        );

    virtual bool waitPacketSent();

    virtual bool waitCAD();

    virtual bool isChannelActive();

    /// Initialise the Driver transport hardware and software.
    /// Make sure the Driver is properly configured before calling init().
    /// \return true if initialisation succeeded.
    virtual bool init();

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

    /// Waits until any previous transmit packet is finished being transmitted with waitPacketSent().
    /// Then loads a message into the transmitter and starts the transmitter. Note that a message length
    /// of 0 is NOT permitted.
    /// \param[in] data Array of data to be sent
    /// \param[in] len Number of bytes of data to send (> 0)
    /// \return true if the message length was valid and it was correctly queued for transmit
    virtual bool send(const uint8_t *data, uint8_t len);

    /// Returns the maximum message length
    /// available in this Driver.
    /// \return The maximum legal message length
    virtual uint8_t maxMessageLength();

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
    const uint16_t startSymbol = 0xb38;

    // The length of the headers we add (To, From, Id, Flags)
    // The headers are inside the payload and are therefore protected by the FCS
    const uint8_t headerLen = 4;

    // Maximum message length (including the headers, byte count and FCS) we are willing to support
    // This is pretty arbitrary
    const uint8_t maxPayloadLen;


    /// Outgoing message bits grouped as 6-bit words
    /// 36 alternating 1/0 bits, followed by 12 bits of start symbol (together called the preamble)
    /// Followed immediately by the 4-6 bit encoded byte count,
    /// message buffer and 2 byte FCS
    /// Each byte from the byte count on is translated into 2x6-bit words
    /// Caution, each symbol is transmitted LSBit first,
    /// but each byte is transmitted high nybble first
    /// This is the number of 6 bit nibbles in the preamble
    const uint8_t preambleLen = 8;

    // This is the maximum message length that can be supported by this library.
    // Can be pre-defined to a smaller size (to save SRAM) prior to including this header
    // Here we allow for 1 byte message length, 4 bytes headers, user data and 2 bytes of FCS
    // maxPayloadLen - headerLen - 3
    const uint8_t maxMsgLen;

    const uint8_t rxSamples;

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

    volatile uint16_t _rxBad;
    volatile uint16_t _rxGood;

    volatile uint16_t _txGood;

    volatile RHMode _mode;
    unsigned int _cad_timeout;

    /// TO header in the last received mesasge
    volatile uint8_t _rxHeaderTo;

    /// FROM header in the last received mesasge
    volatile uint8_t _rxHeaderFrom;

    /// ID header in the last received mesasge
    volatile uint8_t _rxHeaderId;

    /// FLAGS header in the last received mesasge
    volatile uint8_t _rxHeaderFlags;

    /// TO header to send in all messages
    uint8_t _txHeaderTo;

    /// FROM header to send in all messages
    uint8_t _txHeaderFrom;

    /// ID header to send in all messages
    uint8_t _txHeaderId;

    /// FLAGS header to send in all messages
    uint8_t _txHeaderFlags;

    /// Helper function for calculating timer ticks
    uint8_t timerCalc(uint16_t speed, uint16_t max_ticks, uint16_t *nticks);

    /// Set up the timer and its interrutps so the interrupt handler is called at the right frequency
    void timerSetup();

    /// Read the rxPin in a platform dependent way, taking into account whether it is inverted or not
    RH_INTERRUPT_ATTR bool readRx();

    /// Write the txPin in a platform dependent way
    void writeTx(bool value);

    /// Write the txPin in a platform dependent way, taking into account whether it is inverted or not
    void writePtt(bool value);

    /// Translates a 6 bit symbol to its 4 bit plaintext equivalent
    RH_INTERRUPT_ATTR uint8_t symbol_6to4(uint8_t symbol);

    /// The receiver handler function, called a 8 times the bit rate
    void receiveTimer();

    void registerSample(bool rxSample);
    void synchronize();
    bool bitTransition();
    void processBit();

    /// The transmitter handler function, called a 8 times the bit rate
    void transmitTimer();

    /// Check whether the latest received message is complete and uncorrupted
    /// We should always check the FCS at user level, not interrupt level
    /// since it is slow
    void validateRxBuf();

    /// Configure bit rate in bits per second
    uint16_t _speed;

    /// The configure receiver pin
    uint8_t _rxPin;

    /// The configure transmitter pin
    uint8_t _txPin;

    /// The configured transmitter enable pin
    uint8_t _pttPin;

    // Used in the interrupt handlers
    /// Buf is filled but not validated
    volatile bool _rxBufFull;

    /// Buf is full and valid
    volatile bool _rxBufValid;

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

    /// The incoming message buffer
    uint8_t *_rxBuf;

    /// The incoming message expected length
    volatile uint8_t _rxCount;

    /// The incoming message buffer length received so far
    volatile uint8_t _rxBufLen;

    /// Index of the next symbol to send. Ranges from 0 to vw_tx_len
    uint8_t _txIndex;

    /// Bit number of next bit to send
    uint8_t _txBit;

    /// Sample number for the transmitter. Runs 0 to 7 during one bit interval
    uint8_t _txSample;

    /// The transmitter buffer in _symbols_ not data octets
    uint8_t *_txBuf;

    /// Number of symbols in _txBuf to be sent;
    uint8_t _txBufLen;
};

/// @example ask_reliable_datagram_client.ino
/// @example ask_reliable_datagram_server.ino
/// @example ask_transmitter.ino
/// @example ask_receiver.ino
#endif
