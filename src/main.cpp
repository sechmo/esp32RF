#include <Arduino.h>


#define isReceiver false
#define USE_RADIO 1 

const uint8_t inputPin = GPIO_NUM_14;
const uint8_t outputPin = GPIO_NUM_32;
const int ledInternal = GPIO_NUM_2;
const int ledPin0 = GPIO_NUM_23; // amarillo
const int ledPin1 = GPIO_NUM_22; // mroado
const int ledPin2 = GPIO_NUM_21; // naranja
const int ledPin3 = GPIO_NUM_19; // blanco

const int speed = 3000;
#define USE_OWN 1
#include <RadioCoder.h>

RadioCoder driver(speed, inputPin, outputPin, 0); // ESP8266 or ESP32: do not use pin 11 or 2


uint8_t rxBufLen;
uint8_t* rxBuf;

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

    Serial.println("setup done");



}


void loopReceiver()
{

    if (driver.recv(rxBuf, &rxBufLen))
    {
        // Message with a good checksum received, dump it.
        // Serial.print("Got: ");
        // for (int i = 0; i < buflen; i++)
        // {
        //     Serial.print((char)buf[i]);
        // }
        // Serial.println();
        for (int i = 0; i < rxBufLen; i++)
        {
            Serial.print(rxBuf[i]);
        }
        Serial.println();

        digitalWrite(ledInternal, HIGH);
        delay(50);
        digitalWrite(ledInternal, LOW);
        delay(50);
        rxBufLen = driver.maxMessageLength(); // Reset the length for the next message
    }
    // int16_t *data = (int16_t *)rxBuf;
    // // int bytes = available();

    // if (driver.recv(rxBuf, &rxBufLen))
    // {
    //     // Message with a good checksum received, dump it.
    //     // Serial.print("Got: ");
    //     // for (int i = 0; i < buflen; i++)
    //     // {
    //     //     Serial.print((char)buf[i]);
    //     // }
    //     // Serial.println();
    //     for (int i = 0; i < rxBufLen/sizeof(int16_t); i++)
    //     {
    //         Serial.print(data[i]);
    //     }
    //     Serial.println();

    //     digitalWrite(ledPin, HIGH);
    //     delay(50);
    //     digitalWrite(ledPin, LOW);
    //     delay(50);
    //     // buflen = driver.maxMessageLength(); // Reset the length for the next message
    // }


// THIS works

    // int16_t data[1024];
    // size_t dataLen = 1024;

    // // uint8_t bl = driver.maxMessageLength();
    // rxBufLen = driver.maxMessageLength();

    // bool hasMsg = driver.recv(rxBuf, &rxBufLen);

    // dataLen = hasMsg ? rxBufLen : 0;

    // // Serial.println("here");

    // if (dataLen > 0) // Non-blocking
    // {

    //     uint8_t len8 = dataLen < 0 ? 0 : (dataLen > rxBufLen ? rxBufLen : dataLen);
    //     // dataLen = len8;

    //     memcpy(data, rxBuf, len8);


    //     int i;

    //     for (int i = 0; i < dataLen / sizeof(int16_t); i++)
    //     {
    //         Serial.printf("%d:",i);
    //         Serial.println(data[i]);
    //     }

    //     digitalWrite(ledPin, HIGH);
    //     delay(6);
    //     digitalWrite(ledPin, LOW);
    //     delay(6);
    //     // buflen = driver.maxMessageLength(); // Reset the length for the next message
    // }
    // dataLen = 1024;

// --------

/// Also works

    // int16_t data[1024];
    // size_t dataLen = 1024;

    // // uint8_t bl = driver.maxMessageLength();
    // dataLen = available();

    // // Serial.println("here");

    // if (dataLen > 0) // Non-blocking
    // {

    //     dataLen = readBytes((uint8_t *)data, dataLen * sizeof(int16_t));

    //     for (int i = 0; i < dataLen / sizeof(int16_t); i++)
    //     {
    //         Serial.printf("%d:",i);
    //         Serial.println(data[i]);
    //     }

    //     digitalWrite(ledPin, HIGH);
    //     delay(6);
    //     digitalWrite(ledPin, LOW);
    //     delay(6);

    // }

////////////////////
}
void loopTransmitter()
{

    const char* data = "Hello, world!";

    driver.send((uint8_t*)data, strlen(data));
    driver.waitPacketSent();

    Serial.println("Sent");
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

    // digitalWrite(ledPin0, HIGH);
    // delay(200);
    // digitalWrite(ledPin0, LOW);
    // digitalWrite(ledPin1, HIGH);
    // delay(200);
    // digitalWrite(ledPin1, LOW);
    // digitalWrite(ledPin2, HIGH);
    // delay(200);
    // digitalWrite(ledPin2, LOW);
    // digitalWrite(ledPin3, HIGH);
    // delay(200);
    // digitalWrite(ledPin3, LOW);
    // delay(200);

}
