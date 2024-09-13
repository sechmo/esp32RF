// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12

#include <Arduino.h> 

const uint8_t inputPin = GPIO_NUM_33;
const uint8_t outputPin = GPIO_NUM_19;
const int ledPin = GPIO_NUM_2;
const bool isReceiver = true;

const int speed = 100;
#define USE_OWN 0
#if (USE_OWN == 1)

#include <RFSync.h>
// #include <RH_ASK.h>
// #include <SPI.h> // Not actually used but needed to compile

// RH_ASK driver(2000, inputPin, outputPin, 0); // ESP8266 or ESP32: do not use pin 11 or 2

RFSync receiver(speed, inputPin);

uint8_t* buf; 
size_t buflen;

void setup()
{
    pinMode(ledPin, OUTPUT);
    Serial.begin(9600);   // Debugging only
    // if (!driver.init())
    //      Serial.println("init failed");


    if (!receiver.init())
        Serial.println("init failed");

    size_t buflen = receiver.maxMessageLength();
    uint8_t* buf = new uint8_t[buflen];

    Serial.println("setup done");
}
 
void loopReceiver()
{

    // uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    // uint8_t buflen = sizeof(buf);

 
    // if (driver.recv(buf, &buflen)) // Non-blocking
    if (receiver.available())
    {

        if (!receiver.receive(buf, &buflen))
        {
            Serial.println("receive failed");
            return;
        }


 
        // // Message with a good checksum received, dump it.
        // driver.printBuffer("Got:", buf, buflen);

        Serial.print("Got: ");
        for (int i = 0; i < buflen; i++)
        {
            Serial.print((char)buf[i]);
        }
        Serial.println();


        // // flash led to show received message
        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
    }
}

void loopTransmitter()
{

    
    const char *msg = "hello";
    // driver.send((uint8_t *)msg, strlen(msg));
    // driver.waitPacketSent();

    Serial.print("Sent");
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);


    // digitalWrite(ledPin, LOW);
    // delay(2000);

    // // read the serial input and transmit it
    // if (Serial.available())
    // {
    //     // Read the serial data
    //     char data = Serial.read();
    //     // Send the data
    //     driver.send((uint8_t *)&data, 1);
    //     driver.waitPacketSent();
    //     // Print the data to the serial monitor
    //     Serial.print("Sent: ");
    //     Serial.println(data);
    // }
}

void loop()
{
    if (isReceiver)
    {
        if (receiver.enabled()) {
            loopReceiver();
            // Serial.print("testCount Bin: ");
            // Serial.println(receiver.testCount, BIN);
        }
        else {
            delay(1000);
            static int count = 0;
            count++;
            if (count < 10) {
                Serial.print("testCount Bin: ");
                Serial.println(receiver.testCount, BIN);
                Serial.print("testCount Dec: ");
                Serial.println(receiver.testCount);
                Serial.print("testCount Hex: ");
                Serial.println(receiver.testCount, HEX);

                for (int i = 0; i < sizeof(receiver.lastChangeProgresses); i++)
                {
                    Serial.print("lastChangeProgresses[");
                    Serial.print(i);
                    Serial.print("]: ");
                    Serial.println(receiver.lastChangeProgresses[i]);
                }


            } else {
                // receiver.testCount = 88;
                // receiver.enable();
                // count = 0;
            }
        }

    }
    else
    {
        loopTransmitter();
    }
}

#else

#include <RH_ASK.h>
#ifdef RH_HAVE_HARDWARE_SPI
#include <SPI.h> // Not actually used but needed to compile
#endif
 
RH_ASK driver(speed, inputPin, outputPin, 0); // ESP8266 or ESP32: do not use pin 11 or 2
 
void setup()
{
    Serial.begin(9600);   // Debugging only
    if (!driver.init())
         Serial.println("init failed");
    pinMode(ledPin, OUTPUT);

    Serial.println("setup done");

    }
 
void loopReceiver()
{
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t buflen = sizeof(buf);
 
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
        int i;
 
        // Message with a good checksum received, dump it.
        Serial.print("Got: ");
        for (int i = 0; i < buflen; i++)
        {
            Serial.print((char)buf[i]);
        }
        Serial.println();

        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
        delay(50);
    }
}


void loopTransmitter()
{

    
    const char *msg = "hello";
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();

    Serial.print("Sent");
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);


    // digitalWrite(ledPin, LOW);
    // delay(2000);

    // // read the serial input and transmit it
    // if (Serial.available())
    // {
    //     // Read the serial data
    //     char data = Serial.read();
    //     // Send the data
    //     driver.send((uint8_t *)&data, 1);
    //     driver.waitPacketSent();
    //     // Print the data to the serial monitor
    //     Serial.print("Sent: ");
    //     Serial.println(data);
    // }
}


void loop()
{
    if (isReceiver)
    {
        if (driver.enabled()) {
            loopReceiver();
        }
        else {
            delay(1000);
            static int count = 0;
            count++;
            if (count < 10) {
                // Serial.print("testCount Bin: ");
                // Serial.println(receiver.testCount, BIN);
                // Serial.print("testCount Dec: ");
                // Serial.println(receiver.testCount);
                // Serial.print("testCount Hex: ");
                // Serial.println(receiver.testCount, HEX);
                for (int i = 0; i < sizeof(driver.lastChangeProgresses); i++)
                {
                    Serial.print("lastChangeProgresses[");
                    Serial.print(i);
                    Serial.print("]: ");
                    Serial.println(driver.lastChangeProgresses[i]);
                }
            }
        }

        // loopReceiver();
    }
    else
    {
        loopTransmitter();
    }
}


#endif