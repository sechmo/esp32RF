#include <RadioSync.h>


#define RH_BROADCAST_ADDRESS 0xff
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




    /// Returns the maximum message length
    /// available in this Driver.
    /// \return The maximum legal message length
    virtual uint8_t maxMessageLength();

    /// Tests whether a new message is available
    /// from the Driver.
    /// On most drivers, this will also put the Driver into RHModeRx mode until
    /// a message is actually received bythe transport, when it wil be returned to RHModeIdle.
    /// This can be called multiple times in a timeout loop
    /// \return true if a new, complete, error-free uncollected message is available to be retreived by recv()
    virtual bool available();

    // returns the len of the available message if available
    size_t availableLength();

    bool availableToTransmit();

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

    bool waitPacketSent();

    /// Waits until any previous transmit packet is finished being transmitted with waitPacketSent().
    /// Then loads a message into the transmitter and starts the transmitter. Note that a message length
    /// of 0 is NOT permitted.
    /// \param[in] data Array of data to be sent
    /// \param[in] len Number of bytes of data to send (> 0)
    /// \return true if the message length was valid and it was correctly queued for transmit
    virtual bool send(const uint8_t *data, uint8_t len);


protected:

    void validateRxBuf();

    const uint16_t startSymbol = 0xb38;


    bool detectedMsgStart(uint16_t receivedBits) override;

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

    uint8_t decodeByte(uint16_t receivedBits) override;



    /// TO header in the last received mesasge
    volatile uint8_t _rxHeaderTo;

    /// FROM header in the last received mesasge
    volatile uint8_t _rxHeaderFrom;

    /// ID header in the last received mesasge
    volatile uint8_t _rxHeaderId;

    /// FLAGS header in the last received mesasge
    volatile uint8_t _rxHeaderFlags;

    /// Buf is full and valid
    volatile bool _rxBufValid;
    /// Translates a 6 bit symbol to its 4 bit plaintext equivalent
    RH_INTERRUPT_ATTR uint8_t symbol_6to4(uint8_t symbol);


    /// TO header to send in all messages
    uint8_t _txHeaderTo;

    /// FROM header to send in all messages
    uint8_t _txHeaderFrom;

    /// ID header to send in all messages
    uint8_t _txHeaderId;

    /// FLAGS header to send in all messages
    uint8_t _txHeaderFlags;

    bool moreBitsToTransmit() override;

    bool nextBitToTransmit() override;

    void prepareTransmit() override;


    /// Index of the next symbol to send. Ranges from 0 to vw_tx_len
    uint8_t _txIndex;

    /// Bit number of next bit to send
    uint8_t _txBit;
    /// The transmitter buffer in _symbols_ not data octets
    uint8_t *_txBuf;

    /// Number of symbols in _txBuf to be sent;
    uint8_t _txBufLen;
};