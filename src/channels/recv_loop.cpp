#include "shared_resources.h"

PacketComm packetRecv;
char buf[256];
// RXS Loop
void recv_loop(shared_resources *shared)
{
    // Wait for main loop to start
    int32_t iretnRecv = 0;
    delay(1000);
    while (true)
    {
        uint16_t size = 0;
        uint8_t buflen = shared->HWSerial->available();
        packetRecv.wrapped.resize(256);
        char msg[16];
        sprintf(msg, "available: %03d", buflen);
        Serial.println(msg);
        while(!shared->SLIPHWSerial.endofPacket())
        {
        // len is either 0 (done) or 1 (not end)
        uint8_t len = shared->SLIPHWSerial.available();
        // If there are bytes in receive buffer to be read
        if (len > 0)
        {
            // Read a slip packet
            while(len--)
            {
            // Note: goes into wrapped because SLIPHWSerial.read()
            // already decodes SLIP encoding
            packetRecv.wrapped[size] = shared->SLIPHWSerial.read();
            //buf[size] = SLIPHWSerial.read();
            size++;
            }
        }
        }
        packetRecv.wrapped.resize(size);
        // iretnRecv = packetRecv.SLIPUnPacketize();
        iretnRecv = packetRecv.Unwrap();
        Serial.print("iretnRecv: ");
        Serial.print(iretnRecv);
        Serial.print(" size ");
        Serial.print(size);
        Serial.print(" wrapped.size ");
        Serial.print(packetRecv.wrapped.size());
        Serial.print(" data.size ");
        Serial.println(packetRecv.data.size());
        for (size_t i=0; i < packetRecv.wrapped.size(); ++i) {
        Serial.print(packetRecv.wrapped[i]);
        Serial.print(" ");
        }
        Serial.println();
        for (size_t i=0; i < packetRecv.data.size(); ++i) {
        Serial.print(char(packetRecv.data[i]));
        }
        Serial.println();
        Serial.println("now sleeping...");
        threads.delay(3000);
        threads.yield();
    }
}