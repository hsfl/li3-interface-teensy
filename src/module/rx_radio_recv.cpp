#include "module/radio_recv.h"
#include "shared_resources.h"

using namespace Cosmos::Devices::Radios;

// Global shared defined in main.cpp
extern shared_resources shared;

namespace
{
    int32_t iretn = 0;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
    Astrodev::frame incoming_message;
    elapsedMillis last_connected;
    // Ping occasionally to see if RX radio is alive
    elapsedMillis rx_ping_timer = 999999; // ping immediately

    // AX25 header left-shifts the destination callsign by 1,
    // precalculate for faster checks later to
    // discard anything RX hears from the TX radio's transmissions,
    // which unfortunately happens at higher power amp levels.
    char DEST_CALLSIGN_SHIFTED[6];

    // If it has been more than 2 minutes since last radio response 
    const unsigned long unconnected_timeout = 2 * 60 * 1000;
    // Every 45 seconds try checking if RX radio is still alive, and no receive packets have come in
    const unsigned long ping_time = 45000;
}

void Cosmos::Module::Radio_interface::rx_recv_loop()
{
    // int32_t iretn;
    packet.header.nodedest = 0;

    for (uint8_t i=0; i<6; ++i)
    {
        DEST_CALLSIGN_SHIFTED[i] = DEST_CALL_SIGN[i] << 1;
    }
    
    Serial.println("rx_recv_loop started");
    while(true)
    {
        threads.delay(10);

        //Receive
        iretn = shared.astrodev_rx.Receive(incoming_message);
        if (iretn >= 0)
        {
            last_connected = 0;
            Radio_interface::handle_rx_recv(incoming_message);
        }

        // Grab Telemetry every 45 seconds, and check if we are still connected.
        // If receive packets are coming in, then no need to ping.
        if (rx_ping_timer > ping_time)
        {
            // Check connection
            shared.astrodev_rx.Ping(false);
            // shared.astrodev_rx.GetTCVConfig(false);
            // shared.astrodev_rx.GetTelemetry(false);
            rx_ping_timer = 0;
            // Attempt receive of any of the above packets
            continue;
        }

        // Check if we are still connected
        if (last_connected > unconnected_timeout)
        {
            // Reboot if encountering excessive difficulty
            Serial.println("RX Timeout, rebooting...");
            threads.delay(5000);
            WRITE_RESTART(0x5FA0004);
        }
    }
}

void Cosmos::Module::Radio_interface::handle_rx_recv(const Astrodev::frame& msg)
{
    // Handle rx radio messages
    switch (msg.header.command)
    {
    // 0 is Acknowledge-type, view comment on return types for Astrodev::Receive()
    // Though this does mean that I can't distinguish between acks and noacks
    case (Astrodev::Command)0:
        return;
    case Astrodev::Command::NOOP:
    case Astrodev::Command::GETTCVCONFIG:
    case Astrodev::Command::TELEMETRY:
        // Setup PacketComm packet stuff
        packet.header.type = Cosmos::Support::PacketComm::TypeId::CommandRadioAstrodevCommunicate;
        packet.header.nodeorig = IOBC_NODE_ID;
        packet.header.nodedest = IOBC_NODE_ID;
        packet.data.resize(msg.header.sizelo + 4);
        // Byte 0 = Unit (0)
        // Byte 1 = CMD in or out
        // Byte 2 = Astrodev cmd id
        // Byte 3 = Number of response bytes
        packet.data[0] = LI3RX;
        packet.data[1] = 0x20;
        packet.data[2] = (uint8_t)msg.header.command;
        packet.data[3] = msg.header.sizelo;
        memcpy(packet.data.data()+4, &msg.payload[0], msg.header.sizelo);
        break;
        // Don't bother wasting time sending these back
    case Astrodev::Command::SETTCVCONFIG:
    case Astrodev::Command::FASTSETPA:
        return;
    case Astrodev::Command::RECEIVE:
        // Packets from the ground will be in PacketComm protocol
        // TODO: get rid of redundant unwrap/wrap that will probably be happening at sending this back to iobc

        // Receive packet is sufficient evidence of alive-ness
        rx_ping_timer = 0;

        // If the source of the packet is from the TX radio, that's no good, discard.
        if (strncmp((char*)&msg.payload[7], DEST_CALLSIGN_SHIFTED, 6) == 0)
        {
            Serial.println("RX hearing own messages from TX!");
            return;
        }
        Serial.print("in rx receive, bytes:");
        Serial.println(msg.header.sizelo);
        packet.wrapped.resize(msg.header.sizelo);
        memcpy(packet.wrapped.data(), &msg.payload[0], msg.header.sizelo);
        // Let iobc handle everything from ground
        // packet.header.type = Cosmos::Support::PacketComm::TypeId::Blank;
        // packet.header.nodeorig = GROUND_NODE_ID;
        // break;
        shared.send_packet_to_iobc(packet);
        return;
    default:
        Serial.print("rx: cmd ");
        Serial.print((uint16_t)msg.header.command);
        Serial.println(" not (yet) handled. Terminating...");
        // exit(-1);
        return;
    }
#ifdef DEBUG_PRINT
    Serial.print("pushing to main queue, cmd ");
    Serial.println((uint16_t)msg.header.command);
#endif
    Serial.println("sending to queue");
    shared.push_queue(shared.main_queue, shared.main_lock, packet);
}
