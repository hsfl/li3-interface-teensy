#include "module/iobc_recv.h"
#include "shared_resources.h"

// Global shared defined in main.cpp
extern shared_resources shared;

namespace
{
    int32_t iretn = 0;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
}

// Listens for SLIP packets from the iobc and forwards them to the main queue to be handled
void Cosmos::Module::Radio_interface::iobc_recv_loop()
{
    packet.header.dest = 0;
    Serial.println("iobc_recv_loop started");
    
    while (true)
    {
        threads.delay(10);
        Serial.println("I'm here 000");

        uint16_t size = 0;
        packet.wrapped.resize(256);
        while(!shared.SLIPIobcSerial.endofPacket())
        {
            Serial.println(".");
            // len is either 0 (done) or 1 (not end)
            uint8_t len = shared.SLIPIobcSerial.available();
            // If there are bytes in receive buffer to be read
            if (len > 0)
            {
                // Read a slip packet
                while(len--)
                {
                // Note: goes into wrapped because SLIPIobcSerial.read()
                // already decodes SLIP encoding
                packet.wrapped[size] = shared.SLIPIobcSerial.read();
                size++;
                }
            }
        }
        packet.wrapped.resize(size);
        iretn = packet.Unwrap();
        if (iretn < 0)
        {
            Serial.println("Unwrap error");
            continue;
        }
        // Forward to appropriate queue
        switch(packet.header.dest)
        {
        case GROUND_NODE_ID:
            Serial.println("To ground");
            shared.push_queue(shared.send_queue, shared.send_lock, packet);
            break;
        case IOBC_NODE_ID:
            Serial.println("To me");
            shared.push_queue(shared.main_queue, shared.main_lock, packet);
            break;
        default:
            Serial.println("Packet destination not handled, exiting...");
            exit(-1);
            break;
        }
        // Serial.print("iretn: ");
        // Serial.print(iretn);
        // Serial.print(" size ");
        // Serial.print(size);
        // Serial.print(" wrapped.size ");
        // Serial.print(packet.wrapped.size());
        // Serial.print(" data.size ");
        // Serial.println(packet.data.size());
        // for (size_t i=0; i < packet.wrapped.size(); ++i) {
        //     Serial.print(packet.wrapped[i]);
        //     Serial.print(" ");
        // }
        // Serial.println();
        // for (size_t i=0; i < packet.data.size(); ++i) {
        //     Serial.print(char(packet.data[i]));
        // }
        // Serial.println();
        Serial.println("I'm here");
    }
}