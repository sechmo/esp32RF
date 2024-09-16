// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12

#include <Arduino.h>
#include <SPIFFS.h>

#include "AudioTools.h"
#include  <AudioLibs/AudioSourceSPIFFS.h>



AudioInfo info(44100, 1, 16);
SineWaveGenerator<int16_t> wave(16000);
// SquareWaveGenerator<int16_t> wave(16000);
const char *startFilePath="/";
const char* ext="wav";
AudioSourceSPIFFS source(startFilePath, ext);
WAVDecoder decoder;

GeneratedSoundStream<int16_t> sound(wave);
AnalogAudioStream out;
StreamCopy copier(out, sound);

AudioPlayer player(source,out, decoder);
void printMetaData(MetaDataType type, const char* str, int len){
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}



#define USE_RADIO 0 

#if (USE_RADIO == 1)
const uint8_t inputPin = GPIO_NUM_33;
const uint8_t outputPin = GPIO_NUM_19;
const int ledPin = GPIO_NUM_2;
const bool isReceiver = false;

const int speed = 2000;
#define USE_OWN 1
#if (USE_OWN == 1)
#include <RadioCoder.h>

RadioCoder driver(speed, inputPin, outputPin, 0); // ESP8266 or ESP32: do not use pin 11 or 2
#else

#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile
RH_ASK driver(speed, inputPin, outputPin, 0); // ESP8266 or ESP32: do not use pin 11 or 2
#endif


uint8_t *buf;
uint8_t buflen;

#endif

void setup()
{
    Serial.begin(115200); // Debugging only

    #if (USE_RADIO == 1)
    if (!driver.init())
        Serial.println("init failed");
    pinMode(ledPin, OUTPUT);


    buflen = driver.maxMessageLength();

    buf = new uint8_t[buflen];
    #endif


    SPIFFS.begin();




    // AudioLogger::instance().begin(Serial, AudioLogger::Info);

    auto config = out.defaultConfig(TX_MODE);
    config.copyFrom(info);
    out.begin(config);


    wave.begin(info, N_B4);

    // source.setFileFilter("*.wav");
    // player.setMetadataCallback(printMetaData);

    // player.begin();

    copier.begin();


        
    Serial.println("setup done");
}
#if (USE_RADIO == 1)
void loopReceiver()
{

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
#endif

void loop()
{

    #if (USE_RADIO == 1)
    if (isReceiver)
    {
        loopReceiver();
    }
    else
    {
        loopTransmitter();
    }
    #endif

    // copier.copy();
    // if (!player.isActive())
    // {
    //     player.begin();
    // }
    // player.copy();


    copier.copy();


}
