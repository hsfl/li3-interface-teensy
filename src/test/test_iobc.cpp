#include <Arduino.h>
#include <map>
#include <TeensyThreads.h>
#include <SLIPEncodedSerial.h>

#include "common_def.h"
#include "support/packetcomm.h"

#define HWSERIAL Serial1
SLIPEncodedSerial SLIPIobcSerial(HWSERIAL);

namespace
{
    int32_t iretn = 0;
    Cosmos::Support::PacketComm packet;
    uint32_t send_counter = 0;
    elapsedMillis telem_timer;
    elapsedMillis transmit_timer;
}

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

void send_get_telem_packet()
{
    packet.header.type = PacketComm::TypeId::CommandRadioCommunicate;
    packet.header.orig = IOBC_NODE_ID;
    packet.header.dest = IOBC_NODE_ID;
    packet.header.radio = 0;
    packet.data.resize(4);
    // Get rxs telem
    packet.data[0] = LI3RXS;
    packet.data[1] = 7;
    packet.data[2] = 0;
    packet.data[3] = 0;
    iretn = packet.SLIPPacketize();
    if (iretn)
    {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        threads.delay(10);
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        threads.delay(10);
    }
    // Get txs telem
    packet.data[0] = LI3TXS;
    iretn = packet.SLIPPacketize();
    if (iretn)
    {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        threads.delay(10);
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        threads.delay(10);
    }
    packet.data.clear();
    // wait for a bit
    threads.delay(10);
}

void send_test_transmit_packet()
{
    packet.header.type = PacketComm::TypeId::CommandPing;
    packet.header.orig = IOBC_NODE_ID;
    packet.header.dest = GROUND_NODE_ID;
    packet.header.radio = 0;
    const size_t REPEAT = 14;
    packet.data.resize(sizeof(send_counter)*REPEAT);
    for (size_t i = 0; i < REPEAT; ++i)
    {
        memcpy(packet.data.data()+i*sizeof(send_counter), &send_counter, sizeof(send_counter));
    }
    iretn = packet.SLIPPacketize();
    if (iretn)
    {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        threads.delay(10);
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
        Serial.print("wrapped.size:");
        Serial.print(packet.wrapped.size());
        Serial.print(" packetized.size:");
        Serial.println(packet.packetized.size());
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        threads.delay(10);
    }
    packet.data.clear();
    ++send_counter;
    // wait for a bit
    threads.delay(10);
}

void loop()
{
    // Get telem every 5 seconds
    if (telem_timer > 5000)
    {
        telem_timer -= 5000;
        send_get_telem_packet();
    }

    // Send transmit packets every second
    if (transmit_timer > 1000)
    {
        transmit_timer -= 1000;
        send_test_transmit_packet();
    }
    
    threads.delay(10);
}


