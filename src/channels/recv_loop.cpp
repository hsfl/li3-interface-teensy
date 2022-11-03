#include "shared_resources.h"
#include "channels/recv_loop.h"

// Global shared defined in main.cpp
extern shared_resources shared;

namespace
{
    Cosmos::Support::PacketComm packet;
}

char buf[256];
// RXS Loop
void Cosmos::Radio_interface::recv_loop()
{
    // int32_t iretn;
    packet.header.dest = 0;
    Serial.println("recv_loop started");
    // Wait for main loop to start
    delay(1000);
    while (true)
    {
    //     uint16_t size = 0;
    //     uint8_t buflen = shared.HWSerial->available();
    //     packet.wrapped.resize(256);
    //     char msg[16];
    //     sprintf(msg, "available: %03d", buflen);
    //     Serial.println(msg);
    //     while(!shared.SLIPHWSerial.endofPacket())
    //     {
    //     // len is either 0 (done) or 1 (not end)
    //     uint8_t len = shared.SLIPHWSerial.available();
    //     // If there are bytes in receive buffer to be read
    //     if (len > 0)
    //     {
    //         // Read a slip packet
    //         while(len--)
    //         {
    //         // Note: goes into wrapped because SLIPHWSerial.read()
    //         // already decodes SLIP encoding
    //         packet.wrapped[size] = shared.SLIPHWSerial.read();
    //         //buf[size] = SLIPHWSerial.read();
    //         size++;
    //         }
    //     }
    //     }
    //     packet.wrapped.resize(size);
    //     // iretn = packet.SLIPUnPacketize();
    //     iretn = packet.Unwrap();
    //     Serial.print("iretn: ");
    //     Serial.print(iretn);
    //     Serial.print(" size ");
    //     Serial.print(size);
    //     Serial.print(" wrapped.size ");
    //     Serial.print(packet.wrapped.size());
    //     Serial.print(" data.size ");
    //     Serial.println(packet.data.size());
    //     for (size_t i=0; i < packet.wrapped.size(); ++i) {
    //         Serial.print(packet.wrapped[i]);
    //         Serial.print(" ");
    //     }
    //     Serial.println();
    //     for (size_t i=0; i < packet.data.size(); ++i) {
    //         Serial.print(char(packet.data[i]));
    //     }
    //     Serial.println();
        Serial.println("now sleeping...");
        threads.delay(3000);
        threads.yield();
    }
}