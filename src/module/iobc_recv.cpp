#include "module/iobc_recv.h"
#include "module/radio_send.h"
#include "shared_resources.h"

using namespace Cosmos::Devices::Radios;

// Global shared defined in main.cpp
extern shared_resources shared;

void check_tx_radio();
void handle_tx_recv(const Astrodev::frame& msg);
void send_tx_packet();
void read_iobc_packet();
int32_t pop_queue_nolock(Cosmos::Support::PacketComm &packet);
void push_queue_nolock(const Cosmos::Support::PacketComm &packet);

namespace
{
    int32_t iretn = 0;
    // From the TX
    Astrodev::frame incoming_message;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
    // Throttle sends to keep temperature down
    elapsedMillis send_timer;
    // If a SLIP end marker was missed and it keeps waiting, timeout
    elapsedMillis slip_read_timeout;
    // Time since we got any evidence of the TX being alive
    elapsedMillis tx_last_connected;
    // Time out after 1 second
    const uint32_t SLIP_READ_TIMEOUT_MS = 1000;
    // Max size of a UHF TX packet to read
    const size_t MAX_PACKET_SIZE = 256;
    // We know if we got a whole packet if the SLIP END marker was read, handle as successful read
    bool got_slip_end = false;

    // Check if TX is still alive
    elapsedMillis tx_ping_timer = 999999; // Ping immediately
    // If it has been more than 2 minutes since last radio response 
    const unsigned long unconnected_timeout = 2*60 * 1000;
    // Every 45 seconds try checking if TX radio is still alive, and no receive packets have come in
    const unsigned long ping_time = 45*1000;


    // Queue up TX packets. Since it's only handled here, no need for mutexes
    std::deque<Cosmos::Support::PacketComm> tx_queue;
}

// Listens for SLIP packets from the iobc and forwards them to the main queue to be handled
void Cosmos::Module::Radio_interface::iobc_recv_loop()
{
    tx_queue.clear();
    Serial.println("iobc_recv_loop started");
    
    while (true)
    {
        threads.delay(10);

        check_tx_radio();

        threads.delay(10);

        // Check tx queue and send out a packet if conditions are met
        send_tx_packet();

        if (!shared.SLIPIobcSerial.available())
        {
            continue;
        }

        threads.delay(10);

        read_iobc_packet();
    }
}

// Reads from the serial connection to the iobc and either
// queues to the tx queue or the main channel
void read_iobc_packet()
{
    // Reset SLIP read helper vars
    uint16_t size = 0;
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
        return;
    }

    packet.wrapped.resize(size);
    if (!packet.wrapped.size())
    {
        return;
    }
    threads.yield();

    iretn = packet.Unwrap(true);

    // Unwrap failure may indicate that this is an encrypted packet for the ground
    if (iretn < 0)
    {
        Serial.println("IOBC RECV: Unwrap error."); // No encryption for downlink
        // Serial.println("IOBC RECV: Unwrap error, forwarding to ground.");
        // push_queue_nolock(packet);
        return;
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

void check_tx_radio()
{
    if (!shared.get_tx_radio_initialized_state())
    {
        tx_last_connected = 0;
        return;
    }
    // Not going to bother with mutex lock, since I'll never use SetTCVConfig
    if (tx_ping_timer > ping_time)
    {
        // Check connection
        // FM seems to need to send 2 pings to get 1 back.
        shared.astrodev_tx.Ping(false);
        shared.astrodev_tx.Ping(false);
        // shared.astrodev_tx.GetTCVConfig(false);
        // shared.astrodev_tx.GetTelemetry(false);
        tx_ping_timer = 0;
        // Attempt receive of any of the above packets
    }

    // Check if we are still connected
    if (tx_last_connected > unconnected_timeout)
    {
        // Reboot if encountering excessive difficulty
        Serial.println("TX Timeout, rebooting...");
        threads.delay(5000);
        WRITE_RESTART(0x5FA0004);
    }

    iretn = shared.astrodev_tx.Receive(incoming_message);
    if (iretn < 0)
    {
        return;
    }
    // TX radio is still alive
    tx_last_connected = 0;
    handle_tx_recv(incoming_message);
}

void handle_tx_recv(const Astrodev::frame& msg)
{
    // Handle tx radio messages
    switch (msg.header.command)
    {
    // 0 is Acknowledge-type, view comment on return types for Astrodev::Receive()
    // Though this does mean that I can't distinguish between acks and noacks
    case (Astrodev::Command)0:
        return;
    case Astrodev::Command::NOOP:
    case Astrodev::Command::GETTCVCONFIG:
    case Astrodev::Command::TELEMETRY:
        // These are getting sent to iobc, so no need to manually ping
        // Setup PacketComm packet stuff
        packet.header.type = Cosmos::Support::PacketComm::TypeId::CommandRadioAstrodevCommunicate;
        packet.header.nodeorig = IOBC_NODE_ID;
        packet.header.nodedest = IOBC_NODE_ID;
        packet.data.resize(msg.header.sizelo + 4);
        // Byte 0 = Unit (0)
        // Byte 1 = CMD in or out
        // Byte 2 = Astrodev cmd id
        // Byte 3 = Number of response bytes
        packet.data[0] = LI3TX;
        packet.data[1] = 0x20;
        packet.data[2] = (uint8_t)msg.header.command;
        packet.data[3] = msg.header.sizelo;
        memcpy(packet.data.data()+4, &msg.payload[0], msg.header.sizelo);
        break;
        // Don't bother wasting time sending these back
    case Astrodev::Command::SETTCVCONFIG:
    case Astrodev::Command::FASTSETPA:
    case Astrodev::Command::TRANSMIT:
        return;
    case Astrodev::Command::RECEIVE:
        // Discard anything the TX radio picks up
        return;
    default:
        Serial.print("tx: cmd ");
        Serial.print((uint16_t)msg.header.command);
        Serial.println(" not (yet) handled.");
        // exit(-1);
        return;
    }
#ifdef DEBUG_PRINT
    Serial.print("pushing to main queue, cmd ");
    Serial.println((uint16_t)msg.header.command);
#endif
    shared.push_queue(shared.main_queue, shared.main_lock, packet);
}
