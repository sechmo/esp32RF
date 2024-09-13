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

const int speed = 2000;
#define USE_OWN 1
#if (USE_OWN == 1)
#include <RFDriver.h>

#else

// #include <RH_ASK.h>
// #include <SPI.h> // Not actually used but needed to compile
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
    uint8_t buf[maxMsgLen];
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
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);


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
        loopReceiver();
    }
    else
    {
        loopTransmitter();
    }
}

