#include <Arduino.h>


#define isReceiver false

const uint8_t inputPin = GPIO_NUM_14;
const uint8_t outputPin = GPIO_NUM_32;
const int ledInternal = GPIO_NUM_2;
const int ledPin0 = GPIO_NUM_23; // rojo
const int ledPin1 = GPIO_NUM_22; // azul
const int ledPin2 = GPIO_NUM_21; // amarillo
const int ledPin3 = GPIO_NUM_19; // verde

const int speed = 3000;
#define USE_OWN 1
#include <RadioCoder.h>

RadioCoder driver(speed, inputPin, outputPin, 0); 


uint8_t rxBufLen;
uint8_t* rxBuf;



const uint8_t dataLen = 4;
const uint8_t parityLen = 3;
const uint8_t wordLen = dataLen + parityLen;

const uint8_t GeneratorM[dataLen] =
{
    0b1110000,
    0b1001100,
    0b0101010,
    0b1101001
};

const uint8_t GeneratorMTrans[wordLen] = 
{
    0b1101,
    0b1011,
    0b1000,
    0b0111,
    0b0100,
    0b0010,
    0b0001
};

const uint8_t ParityCheckM[parityLen] =
{
    // constructed in such a way that 
    // if we know the syndrome, we can know the 
    // corresponding column in the parity check matrix
    // by subtracting one (0-based index)

    0b1010101,
    0b0110011,
    0b0001111
};

const uint8_t RecoveryMatrix[dataLen] = 
{
    0b0010000,
    0b0000100,
    0b0000010,
    0b0000001
};

uint8_t paritySum(uint8_t data)
{
    uint8_t sum = 0;
    for (int i = 0; i < wordLen; i++)
    {
        if (data & (1 << i))
        {
            sum++;
        }
    }
    return sum & 1;
}

uint8_t hammingEncode(uint8_t data)
{
    uint8_t code = 0;

    for (int i = 0; i < wordLen; i++)
    {
        // Serial.println();
        // Serial.print(GeneratorMTrans[i],BIN);
        // Serial.print(" & ");
        // Serial.print(data,BIN);
        uint8_t rowProd = (GeneratorMTrans[i] & data);  // and is base 2 prod
        // Serial.print(" = ");
        // Serial.print(rowProd,BIN);
        // Serial.print(" -> ");
        // Serial.println(paritySum(rowProd),BIN);
        code |= paritySum(rowProd) << (wordLen - i - 1);
        // Serial.print("res ");
        // Serial.println(code,BIN);
    }
    // Serial.print("res ");
    // Serial.println(code,BIN);

    return code;
}

uint8_t hammingDecode(uint8_t code, bool* error)
{
    uint8_t syndrome = 0;
    for (int i = 0; i < parityLen; i++)
    {
        uint8_t rowProd = (ParityCheckM[i] & code);
        syndrome |= paritySum(rowProd) << (parityLen - i - 1);
    }

    // Serial.println();
    // Serial.print("syndrome ");
    // Serial.println(syndrome,BIN);

    // Serial.print("code ");
    // Serial.println(code,BIN);   

    if (syndrome != 0)
    {
        *error = true;
        code =  code ^ (1 << (wordLen - (syndrome)));
    }

    *error = false;

    // Serial.print("code ");
    // Serial.println(code,BIN);



    uint8_t decoded = 0;

    for (int i = 0; i < dataLen; i++)
    {
        uint8_t rowProd = (RecoveryMatrix[i] & code);
        decoded |= paritySum(rowProd) << (dataLen - i - 1);
    }

    return decoded;
}

void setup()
{
    Serial.begin(115200); // Debugging only

    if (!driver.init())
        Serial.println("init failed");
    
    pinMode(ledInternal, OUTPUT);
    pinMode(ledPin0, OUTPUT);
    pinMode(ledPin1, OUTPUT);
    pinMode(ledPin2, OUTPUT);
    pinMode(ledPin3, OUTPUT);


    rxBufLen = driver.maxMessageLength();

    rxBuf = new uint8_t[rxBufLen];

    // Serial.println("setup done");

    // for (int i = 0; i < 16; i++)
    // {
    //     uint8_t val = i;
    //     Serial.print(val,BIN);
    //     uint8_t encoded = hammingEncode(val);
    //     Serial.print(" -> ");
    //     Serial.print(encoded,BIN);
    //     uint8_t encoded_err = encoded ^ 0b1000100;
    //     Serial.print(" -> ");
    //     Serial.print(encoded_err,BIN);
    //     uint8_t decoded = hammingDecode(encoded_err);
    //     Serial.print(" -> ");
    //     Serial.println(decoded,BIN);
    // }


}

void setPins(uint8_t ref)
{
    digitalWrite(ledPin0, ref & 0x01);
    digitalWrite(ledPin1, ref & 0x02);
    digitalWrite(ledPin2, ref & 0x04);
    digitalWrite(ledPin3, ref & 0x08);

}

uint8_t randomPinsState()
{
    return random(0, 16);
}


void loopReceiver()
{
    static int errCount = 0;
    bool error = false;

    if (driver.recv(rxBuf, &rxBufLen))
    {
        for (int i = 0; i < rxBufLen; i++)
        {
            Serial.print(rxBuf[i], BIN);
            // decode 
            uint8_t decoded = hammingDecode(rxBuf[i], &error);
            rxBuf[i] = decoded;
            Serial.print(" -> ");
            Serial.print(decoded, BIN);
            if (error) {
                errCount++;
                Serial.print(" (error)");
            } 
        }
        Serial.println();

        digitalWrite(ledInternal, HIGH);
        delay(50);
        digitalWrite(ledInternal, LOW);
        delay(50);
        setPins(rxBuf[0]);
        rxBufLen = driver.maxMessageLength(); // Reset the length for the next message
    }
}
void loopTransmitter()
{

    uint8_t data[1];

    data[0] = randomPinsState();

    setPins(data[0]);

    data[0] = hammingEncode(data[0]);

    driver.send(data, 1);
    driver.waitPacketSent();

    Serial.println("Sent");
    delay(1000);
}
void loop()
{

    if (isReceiver)
    {
        loopReceiver();
    }
    else
    {
        loopTransmitter();
    }

}
