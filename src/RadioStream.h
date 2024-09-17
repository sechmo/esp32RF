// #pragma once

#include <AudioTools/AudioStreams.h>
#include "RadioCoder.h"

#include "AudioTools.h"


class RadioStream : public AudioStream {

public: 
    RadioStream() {
        driver = new RadioCoder(2000, GPIO_NUM_33, GPIO_NUM_19);
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
        return driver->availableLength();
    }

    size_t readBytes(uint8_t* data, size_t len) override {
        TRACEI();



        if (!driver->available()) {
            return 0;
        }


        uint8_t len8 = len < 0 ? 0 : (len > 255 ? 255 : len);

        if (!driver->recv(data, &len8)) {
            LOGE("Failed to receive message");
            return 0;
        }

        return len8;
    }


protected:

    RadioCoder* driver;
    int bufLen;
    uint8_t* buf;

};