#include <Arduino.h>
#include <map>
#include <TeensyThreads.h>
#include <SLIPEncodedSerial.h>

#include "common_def.h"
#include "support/packetcomm.h"

#define HWSERIAL Serial1
SLIPEncodedSerial SLIPIobcSerial(HWSERIAL);

void setup()
{
    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // Setup serial stuff
    Serial.begin(115200);
    SLIPIobcSerial.begin(115200);
    SLIPIobcSerial.flush();

    // Each thread tick length
    threads.setSliceMicros(10);

    threads.delay(2000);
}

int32_t iretn;
Cosmos::Support::PacketComm packet;

void loop()
{
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("sending!");
    packet.header.type = PacketComm::TypeId::CommandPing;
    packet.header.orig = IOBC_NODE_ID;
    packet.header.dest = IOBC_NODE_ID;
    packet.header.radio = 0;
    char str[12] = "hello world";
    packet.data.insert(packet.data.end(), str, str + 12);
    iretn = packet.SLIPPacketize();
    if (iretn)
    {
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
    }
    packet.data.clear();
    // wait for a bit
    threads.delay(10);
    // turn the LED off by making the voltage LOW
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Sleeping...");
    threads.delay(4000);
}