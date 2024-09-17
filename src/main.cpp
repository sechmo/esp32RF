// ask_receiver.pde
// -*- mode: C++ -*-
// Simple example of how to use RadioHead to receive messages
// with a simple ASK transmitter in a very simple way.
// Implements a simplex (one-way) receiver with an Rx-B1 module
// Tested on Arduino Mega, Duemilanova, Uno, Due, Teensy, ESP-12

#include <Arduino.h>
// #include <SPIFFS.h>


#define isReceiver true
#define USE_RADIO 1 
// const bool isReceiver = true;

// const char *startFilePath="/";
// const char* ext="wav";
// AudioSourceSPIFFS source(startFilePath, ext);
// WAVDecoder decoder;


// FileLoop floop;
// EncodedAudioStream decoded(&floop, new WAVDecoder());

// AudioWAVServer server("_", "_", 3333);
// AnalogAudioStream in;

// StreamCopy copier(radio, sound);



// CsvOutput<int16_t> csv(Serial);
// EncodedAudioStream wav(&csv, new WAVEncoder());

// ConverterFillLeftAndRight<int16_t> filler(LeftIsEmpty); // fill both channels - or change to RightIsEmpty


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




#if (USE_RADIO == 1)
const uint8_t inputPin = GPIO_NUM_33;
const uint8_t outputPin = GPIO_NUM_19;
const int ledPin = GPIO_NUM_2;

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


uint8_t rxBufLen;
uint8_t* rxBuf;
#else 


#include "AudioTools.h"
// #include  <AudioLibs/AudioSourceSPIFFS.h>
// #include  <AudioLibs/FileLoop.h>
#include "RadioStream.h"

AudioInfo info(2000, 1, 16);
SineWaveGenerator<int16_t> wave(16000);
GeneratedSoundStream<int16_t> sound(wave);
RadioStream radio;
AnalogAudioStream out;
#if (isReceiver)
StreamCopy copier(out,radio);
#else
StreamCopy copier(radio,sound);
#endif

#endif

void setup()
{
    Serial.begin(115200); // Debugging only

    #if (USE_RADIO == 1)
    if (!driver.init())
        Serial.println("init failed");
    pinMode(ledPin, OUTPUT);


    rxBufLen = driver.maxMessageLength();

    rxBuf = new uint8_t[rxBufLen];
    #else


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

#if (!isReceiver)
    sound.begin(info);
    wave.begin(info, N_B4);
#else
    auto outConfig = out.defaultConfig(TX_MODE);
    outConfig.copyFrom(info);
    out.begin(outConfig);
#endif



    copier.begin(); 





#endif








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



int available() {

    // LOGI("Available: %d", driver->available());
    // LOGI("Ramp: %d", driver->getRamp());
    // return driver->availableLength();

    rxBufLen = driver.maxMessageLength();

    bool hasMsg  = driver.recv(rxBuf, &rxBufLen) ;

    return hasMsg ? rxBufLen: 0;
}

size_t readBytes(uint8_t* data, size_t len)  {



    // if (!driver->available()) {
    //     return 0;
    // }


    uint8_t len8 = len < 0 ? 0 : (len > rxBufLen ? rxBufLen : len);

    memcpy(data, rxBuf, len8);

    // if (!driver->recv(data, &len8)) {
    //     LOGE("Failed to receive message");
    //     return 0;
    // }

    rxBufLen = driver.maxMessageLength();

    return len8;
}


void loopReceiver()
{
    int16_t *data = (int16_t *)rxBuf;
    // int bytes = available();

    if (driver.recv(rxBuf, &rxBufLen))
    {
        // Message with a good checksum received, dump it.
        // Serial.print("Got: ");
        // for (int i = 0; i < buflen; i++)
        // {
        //     Serial.print((char)buf[i]);
        // }
        // Serial.println();
        for (int i = 0; i < rxBufLen/sizeof(int16_t); i++)
        {
            Serial.print(data[i]);
        }
        Serial.println();
        // buflen = driver.maxMessageLength(); // Reset the length for the next message
    }


    // int16_t data[1024];
    // size_t dataLen = 1024;

    // // uint8_t bl = driver.maxMessageLength();
    // rxBufLen = driver.maxMessageLength();

    // bool hasMsg = driver.recv(rxBuf, &rxBufLen);

    // dataLen = hasMsg ? rxBufLen : 0;

    // // Serial.println("here");

    // if (hasMsg) // Non-blocking
    // {

    //     // uint8_t len8 = dataLen < 0 ? 0 : (dataLen > rxBufLen ? rxBufLen : dataLen);

    //     memcpy(data, rxBuf, dataLen);

    //     // dataLen = len8;

    //     int i;

    //     // Message with a good checksum received, dump it.
    //     // Serial.print("Got: ");
    //     // for (int i = 0; i < buflen; i++)
    //     // {
    //     //     Serial.print((char)buf[i]);
    //     // }
    //     // Serial.println();
    //     for (int i = 0; i < dataLen / sizeof(int16_t); i++)
    //     {
    //         Serial.println(data[i]);
    //         // Serial.print("");
    //     }

    //     digitalWrite(ledPin, HIGH);
    //     delay(50);
    //     digitalWrite(ledPin, LOW);
    //     delay(50);
    //     // buflen = driver.maxMessageLength(); // Reset the length for the next message
    // }
    // dataLen = 1024;

    // int bytes = available();
    // int16_t data[1024];
    // uint8_t dataLen = 1024;

    // // Serial.println(bytes);
    // if (bytes > 0)
    // {
    //     readBytes((uint8_t *)data, bytes);
    //     for (int i = 0; i < bytes / sizeof(int16_t); i++)
    //     {
    //         Serial.println(data[i]);
    //     }
    // }
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
#else 

void loopReceiver()
{
    // copier.copy();
    int bytes = radio.available();
    int16_t data[1024];

    Serial.println(bytes);
    if (bytes > 0)
    {
        radio.readBytes((uint8_t *)data, bytes);
        for (int i = 0; i < bytes / sizeof(int16_t); i++)
        {
            Serial.println(data[i]);
        }
    }
}

void loopTransmitter()
{
    copier.copy();
}
#endif

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
