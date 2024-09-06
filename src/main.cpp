// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12
#include <Arduino.h> 
#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
 

const int inputPin = GPIO_NUM_33;
const int outputPin = GPIO_NUM_19;
const int ledPin = GPIO_NUM_2;
const bool isReceiver = true;
 
RH_ASK driver(2000, inputPin, outputPin, 0); // ESP8266 or ESP32: do not use pin 11 or 2
// RH_ASK driver(2000, 3, 4, 0); // ATTiny, RX on D3 (pin 2 on attiny85) TX on D4 (pin 3 on attiny85), 
// RH_ASK driver(2000, PD14, PD13, 0); STM32F4 Discovery: see tx and rx on Orange and Red LEDS

void setup()
{

    pinMode(ledPin, OUTPUT);
    Serial.begin(9600);   // Debugging only
    if (!driver.init())
         Serial.println("init failed");

}
 
void loopReceiver()
{
    uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
    uint8_t buflen = sizeof(buf);
 
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
        int i;
 
        // Message with a good checksum received, dump it.
        // driver.printBuffer("Got:", buf, buflen);

        Serial.print("Got: ");
        for (i = 0; i < buflen; i++)
        {
            Serial.print((char)buf[i]);
        }
        Serial.println();


        // flash led to show received message
        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
    }
}

void loopTransmitter()
{

    
    const char *msg = "hello";
    driver.send((uint8_t *)msg, strlen(msg));
    driver.waitPacketSent();

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
        loopReceiver();
    }
    else
    {
        loopTransmitter();
    }
}