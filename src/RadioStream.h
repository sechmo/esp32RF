// #pragma once

#include <AudioTools/AudioStreams.h>
#include "RadioCoder.h"

#include "AudioTools.h"


class RadioStream : public AudioStream {

public: 
    RadioStream() {
        driver = new RadioCoder(2000, GPIO_NUM_33, GPIO_NUM_25);
    }

    bool begin() {
        TRACEI();
        if (!driver->init()) {
            LOGE("init failed");
            return false;
        }

        bufLen = driver->maxMessageLength();
        buf = new uint8_t[bufLen];

        pinMode(GPIO_NUM_2, OUTPUT);
        return true;
    }

    void end() {
        TRACEI();
    }

    void setAudioInfo(AudioInfo newInfo) override {
        TRACEI();
        AudioStream::setAudioInfo(newInfo);
    }

    int availableForWrite() override {
        TRACEI();
        return driver->maxMessageLength();
    }

    size_t write(const uint8_t* data, size_t len) override {
        TRACEI();

        if (len > driver->maxMessageLength()) {
            len = driver->maxMessageLength();
        }
        bool sent = driver->send(data, len);
        driver->waitPacketSent();
        if (!sent) {
            LOGE("Failed to send message");
            return 0;
        }

        return len;
    }

    int available() override {
        TRACEI();

        // LOGI("Available: %d", driver->available());
        // LOGI("Ramp: %d", driver->getRamp());
        // return driver->availableLength();


        bool hasMsg  = driver->recv(buf, &bufLen) ;


        return hasMsg ? bufLen: 0;
    }

    size_t readBytes(uint8_t* data, size_t len) override {
        TRACEI();



        // if (!driver->available()) {
        //     return 0;
        // }


        uint8_t len8 = len < 0 ? 0 : (len > bufLen ? bufLen : len);

        memcpy(data, buf, len8);

        // if (!driver->recv(data, &len8)) {
        //     LOGE("Failed to receive message");
        //     return 0;
        // }

        bufLen = driver->maxMessageLength();

        return len8;
    }


protected:

    RadioCoder* driver;
    uint8_t bufLen;
    uint8_t* buf;

};