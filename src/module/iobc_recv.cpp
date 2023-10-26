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
    Serial.println("iobc_recv_loop started");
    
    while (true)
    {
        threads.delay(10);

        uint16_t size = 0;
        const size_t MAX_PACKET_SIZE = 256;
        packet.wrapped.resize(MAX_PACKET_SIZE);
        // Continuously read from serial buffer until a packet is received
        while(!shared.SLIPIobcSerial.endofPacket() && size < MAX_PACKET_SIZE)
        {
            // If there are bytes in receive buffer to be read
            if (shared.SLIPIobcSerial.available())
            {
                // Read a slip packet
                // Note: goes into wrapped because SLIPIobcSerial.read()
                // already decodes SLIP encoding
                packet.wrapped[size] = shared.SLIPIobcSerial.read();
                size++;
            } else {
                threads.yield();
            }
        }
        packet.wrapped.resize(size);
        if (!packet.wrapped.size())
        {
            continue;
        }
        threads.yield();

        iretn = packet.Unwrap(true);

        // Unwrap failure may indicate that this is an encrypted packet for the ground
        if (iretn < 0)
        {
            Serial.println("Unwrap error, forwarding to ground.");
            packet.header.nodeorig = IOBC_NODE_ID;
            packet.header.nodedest = GROUND_NODE_ID;
            packet.header.type = Cosmos::Support::PacketComm::TypeId::Blank;
            shared.push_queue(shared.send_queue, shared.send_lock, packet);
            continue;
        }

        // Forward to appropriate queue
        switch(packet.header.nodedest)
        {
        case GROUND_NODE_ID:
            Serial.println("To ground");
            shared.push_queue(shared.send_queue, shared.send_lock, packet);
            break;
        case IOBC_NODE_ID:
            // Tag this packet to be handled internally
            packet.header.nodedest = LI3TEENSY_ID;
            Serial.println("To me");
            Serial.print("Packet header type:");
            Serial.println((uint16_t)packet.header.type);
            shared.push_queue(shared.main_queue, shared.main_lock, packet);
            break;
        case UNIBAP_NODE_ID:
        default:
            Serial.println("Packet destination not handled");
            // exit(-1);
            break;
        }
#ifdef DEBUG_PRINT
        Serial.print("iretn: ");
        Serial.print(iretn);
        Serial.print(" size ");
        Serial.print(size);
        Serial.print(" wrapped.size ");
        Serial.print(packet.wrapped.size());
        Serial.print(" data.size ");
        Serial.println(packet.data.size());
        for (size_t i=0; i < packet.wrapped.size(); ++i) {
            Serial.print(packet.wrapped[i]);
            Serial.print(" ");
        }
        Serial.println();
        for (size_t i=0; i < packet.data.size(); ++i) {
            Serial.print(char(packet.data[i]));
        }
        Serial.println();
#endif
    }
}