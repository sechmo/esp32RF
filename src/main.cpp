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

    if (driver.recv(rxBuf, &rxBufLen))
    {
        for (int i = 0; i < rxBufLen; i++)
        {
            Serial.print((char)rxBuf[i]);
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
