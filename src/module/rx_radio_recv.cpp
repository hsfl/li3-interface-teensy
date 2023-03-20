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
    elapsedMillis rx_telem_timer;

    // If it has been more than 30 seconds since last 
    const unsigned long unconnected_timeout = 30 * 1000;
}

void Cosmos::Module::Radio_interface::rx_recv_loop()
{
    // int32_t iretn;
    packet.header.nodedest = 0;
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

        // Grab Telemetry every 10 seconds, and check if we are still connected
        if (rx_telem_timer > 10000)
        {
            // Check connection
            // shared.astrodev_rx.Ping(false);
            // shared.astrodev_rx.GetTCVConfig(false);
            shared.astrodev_rx.GetTelemetry(false);
            rx_telem_timer = 0;
            // Attempt receive of any of the above packets
            continue;
        }

        // Check if we are still connected
        if (last_connected > unconnected_timeout)
        {
            // Handle not-connnected status
            iretn = shared.connect_radio(shared.astrodev_rx, 449900, 449900);
            if (iretn < 0)
            {
                // Keep attempting reconnect
                continue;
            }
            last_connected = 0;
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
#ifndef MOCK_TESTING
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
    case Astrodev::Command::RECEIVE:
        // Packets from the ground will be in PacketComm protocol
        // TODO: get rid of redundant unwrap/wrap that will probably be happening at sending this back to iobc
        Serial.print("in rx receive, bytes:");
        Serial.println(msg.header.sizelo);
        packet.wrapped.resize(msg.header.sizelo-18);
        memcpy(packet.wrapped.data(), &msg.payload[16], msg.header.sizelo-18);
        iretn = packet.Unwrap();
        if (iretn < 0)
        {
            Serial.println("Unwrap fail in RX radio recv");
            return;
        }
        break;
#else
    // These cases here are for faking a radio interaction.
    // These will be sent from the radio stack interface board's teensy.
    // Any RECEIVE
    case Astrodev::Command::RESET:
    case Astrodev::Command::NOOP:
    case Astrodev::Command::SETTCVCONFIG:
    case Astrodev::Command::GETTCVCONFIG:
        packet.header.type = Cosmos::Support::PacketComm::TypeId::DataRadioResponse;
        packet.data.resize(1);
        packet.data[0] = (uint8_t)msg.header.command;
        break;
#endif
    default:
        Serial.print("cmd ");
        Serial.print((uint16_t)msg.header.command);
        Serial.println(" not (yet) handled. Terminating...");
        // exit(-1);
        return;
    }
#ifdef DEBUG_PRINT
    Serial.print("pushing to main queue, cmd ");
    Serial.println((uint16_t)msg.header.command);
#endif
    shared.push_queue(shared.main_queue, shared.main_lock, packet);
}
