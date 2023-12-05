#include "module/iobc_recv.h"
#include "module/radio_send.h"
#include "shared_resources.h"

// Global shared defined in main.cpp
extern shared_resources shared;

void send_tx_packet();
int32_t pop_queue_nolock(Cosmos::Support::PacketComm &packet);
void push_queue_nolock(const Cosmos::Support::PacketComm &packet);

namespace
{
    int32_t iretn = 0;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
    // Throttle sends to keep temperature down
    elapsedMillis send_timer;
    // If a SLIP end marker was missed and it keeps waiting, timeout
    elapsedMillis slip_read_timeout;
    // Time out after 1 second
    const uint32_t SLIP_READ_TIMEOUT_MS = 1000;
    // Max size of a UHF TX packet to read
    const size_t MAX_PACKET_SIZE = 256;
    // We know if we got a whole packet if the SLIP END marker was read, handle as successful read
    bool got_slip_end = false;


    // Queue up TX packets. Since it's only handled here, no need for mutexes
    std::deque<Cosmos::Support::PacketComm> tx_queue;
}

// Listens for SLIP packets from the iobc and forwards them to the main queue to be handled
void Cosmos::Module::Radio_interface::iobc_recv_loop()
{
    tx_queue.clear();
    Serial.println("iobc_recv_loop started");

    uint16_t size = 0;
    
    while (true)
    {
        threads.delay(10);

        // Check tx queue and send out a packet if conditions are met
        send_tx_packet();

        if (!shared.SLIPIobcSerial.available())
        {
            continue;
        }

        threads.delay(10);

        // Reset SLIP read helper vars
        size = 0;
        packet.wrapped.resize(MAX_PACKET_SIZE, 0);
        got_slip_end = false;
        slip_read_timeout = 0;
        // Receive tx packets or teensy/radio commands from iobc
        // Continuously read from serial buffer until a packet is received
        while(size < MAX_PACKET_SIZE)
        {
            // Keep reading until SLIP END marker is read
            got_slip_end = shared.SLIPIobcSerial.endofPacket();
            if (got_slip_end)
            {
                break;
            }
            // If there are bytes in receive buffer to be read
            if (shared.SLIPIobcSerial.available())
            {
                // Read a slip packet
                // Note: goes into wrapped because SLIPIobcSerial.read()
                // already decodes SLIP encoding
                packet.wrapped[size] = shared.SLIPIobcSerial.read();
                size++;
            }
            // Keep reading until timeout
            else if (slip_read_timeout < SLIP_READ_TIMEOUT_MS)
            {
                threads.yield();
            }
            // Timed out
            else
            {
                break;
            }
        }
        if (!got_slip_end)
        {
            continue;
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
            Serial.println("IOBC RECV: Unwrap error."); // No encryption for downlink
            // Serial.println("IOBC RECV: Unwrap error, forwarding to ground.");
            // push_queue_nolock(packet);
            continue;
        }

        // Forward to appropriate queue
        switch(packet.header.nodedest)
        {
        case GROUND_NODE_ID:
            Serial.println("IOBC RECV: To ground");
            push_queue_nolock(packet);
            break;
        // An IOBC destination packet will come from the iobc
        // only in the event that it's meant as an internal command
        case IOBC_NODE_ID:
            // Tag this packet to be handled internally
            packet.header.nodedest = LI3TEENSY_ID;
            Serial.print("IOBC RECV: To me, Packet header type:");
            Serial.println((uint16_t)packet.header.type);
            shared.push_queue(shared.main_queue, shared.main_lock, packet);
            break;
        default:
            Serial.println("IOBC RECV: Packet destination not handled");
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
        return;
    }
    // Since RX starts hearing TX packets at higher power amp levels,
    // silence TX if RX is not initialized, since RX initialization
    // requires some quietness.
    if (!shared.get_rx_radio_initialized_state())
    {
        return;
    }
    // Throttle to minimize heat
    if (send_timer < TX_THROTTLE_MS)
    {
        return;
    }
    if (pop_queue_nolock(packet) < 0)
    {
        return;
    }
    iretn = Cosmos::Module::Radio_interface::send_packet(packet);
    if (iretn < 0)
    {
        Serial.print("IOBC RECV: Error sending to ground: ");
        Serial.println(iretn);
    }
    else
    {
        send_timer = 0;
    }
}


int32_t pop_queue_nolock(Cosmos::Support::PacketComm &packet)
{
  if (!tx_queue.size())
  {
      return -1;
  }
  packet = tx_queue.front();
  tx_queue.pop_front();
  return tx_queue.size();
}

// Push a packet onto a queue
void push_queue_nolock(const Cosmos::Support::PacketComm &packet)
{
  if (tx_queue.size() > MAX_QUEUE_SIZE)
  {
      tx_queue.pop_front();
  }
  tx_queue.push_back(packet);
}
