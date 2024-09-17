// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12

#include <Arduino.h>
// #include <SPIFFS.h>

#include "AudioTools.h"
// #include  <AudioLibs/AudioSourceSPIFFS.h>
// #include  <AudioLibs/FileLoop.h>
#include "RadioStream.h"




AudioInfo info(2000, 1, 16);
// SineWaveGenerator<int16_t> wave(16000);
SquareWaveGenerator<int16_t> wave(16000);
// const char *startFilePath="/";
// const char* ext="wav";
// AudioSourceSPIFFS source(startFilePath, ext);
// WAVDecoder decoder;


// FileLoop floop;
// EncodedAudioStream decoded(&floop, new WAVDecoder());

GeneratedSoundStream<int16_t> sound(wave);

// AudioWAVServer server("_", "_", 3333);
// AnalogAudioStream in;
RadioStream radio;

StreamCopy copier(radio, sound);

// CsvOutput<int16_t> csv(Serial);
// EncodedAudioStream wav(&csv, new WAVEncoder());

// ConverterFillLeftAndRight<int16_t> filler(LeftIsEmpty); // fill both channels - or change to RightIsEmpty


// AnalogAudioStream out;
// StreamCopy copier(out, sound);
// StreamCopy copier(out, floop);
// StreamCopy copier(wav, in);


// AudioPlayer player(source,out, decoder);
// void printMetaData(MetaDataType type, const char* str, int len){
//   Serial.print("==> ");
//   Serial.print(toStr(type));
//   Serial.print(": ");
//   Serial.println(str);
// }



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


    // SPIFFS.begin();

    // File f = SPIFFS.open("/sample.wav", "r");

    // floop.setFile(f);
    // floop.begin();

    // decoded.begin();
    AudioLogger::instance().begin(Serial, AudioLogger::Debug);


    if (!radio.begin())
    {
        Serial.println("radio begin failed");
    }

    sound.begin(info);
    wave.begin(info, N_B4);

    copier.begin(); 











    // auto outConfig = out.defaultConfig(TX_MODE);
    // outConfig.copyFrom(info);
    // out.begin(outConfig);


    // auto inConfig = in.defaultConfig(RX_MODE);
    // inConfig.copyFrom(info);
    // in.begin(inConfig);

    // wav.begin();

    // wave.begin(info, N_B4);

    // source.setFileFilter("*.wav");
    // player.setMetadataCallback(printMetaData);

    // player.begin();

    // copier.begin();

    // server.begin(in, inConfig, nullptr);


        
    // Serial.println("setup done");
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


    // copier.copy();
    // server.copy();
    copier.copy();


}
