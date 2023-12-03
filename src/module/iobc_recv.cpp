#include "module/iobc_recv.h"
#include "module/radio_send.h"
#include "shared_resources.h"

// Global shared defined in main.cpp
extern shared_resources shared;

void send_tx_packet();

namespace
{
    int32_t iretn = 0;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
    // Throttle sends to keep temperature down
    elapsedMillis send_timer;
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
            send_tx_packet();
            continue;
        }

        // Forward to appropriate queue
        switch(packet.header.nodedest)
        {
        case GROUND_NODE_ID:
            Serial.println("To ground");
            send_tx_packet();
            break;
        // An IOBC destination packet will come from the iobc
        // only in the event that it's meant as an internal command
        case IOBC_NODE_ID:
            // Tag this packet to be handled internally
            packet.header.nodedest = LI3TEENSY_ID;
            Serial.print("To me, Packet header type:");
            Serial.println((uint16_t)packet.header.type);
            shared.push_queue(shared.main_queue, shared.main_lock, packet);
            break;
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

// Send out a TX packet out the TX radio
// Checks radio initialization states and also throttles transmission rate
void send_tx_packet()
{
    if (!shared.get_tx_radio_initialized_state())
    {
        Serial.println("TX not initialized, discarding TX packet...");
        return;
    }
    // Since RX starts hearing TX packets at higher power amp levels,
    // silence TX if RX is not initialized, since RX initialization
    // requires some quietness.
    if (!shared.get_rx_radio_initialized_state())
    {
        Serial.println("RX not initialized, discarding TX packet...");
        return;
    }
    // Throttle to minimize heat
    if (send_timer < TX_THROTTLE_MS)
    {
        // The point of this is to not get bogged down in TX packets
        // if there might be other non-TX packets waiting.
        // Also, for HyTI, UHF down is only for LEOPS telems, so
        // better to discard older telems in favor of newer ones.
        if (DISCARD_THROTTLED_PACKETS)
        {
            Serial.println("DISCARD_THROTTLED_PACKETS true, discarding TX packet.");
            return;
        }
        threads.delay(TX_THROTTLE_MS - send_timer);
    }
    iretn = Cosmos::Module::Radio_interface::send_packet(packet);
    if (iretn < 0)
    {
        Serial.print("Error sending to ground: ");
        Serial.println(iretn);
    }
    else
    {
        send_timer = 0;
    }
}