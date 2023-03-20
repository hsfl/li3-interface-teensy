#include "module/tx_radio_loop.h"
#include "shared_resources.h"

using namespace Cosmos::Devices::Radios;

// Global shared defined in main.cpp
extern shared_resources shared;

namespace
{
    int32_t iretn = 0;
    elapsedMillis last_connected;
    elapsedMillis telem_timer;

    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
    Astrodev::frame incoming_message;

    // If it has been more than 30 seconds since last 
    const unsigned long unconnected_timeout = 30 * 1000;

    const uint8_t max_retries = 3;
}

void Cosmos::Module::Radio_interface::tx_radio_loop()
{
    while(true)
    {
        threads.delay(10);

        //Receive
        iretn = shared.astrodev_tx.Receive(incoming_message);
        if (iretn >= 0)
        {
            last_connected = 0;
            Radio_interface::handle_tx_recv(incoming_message);
        }

        // Grab Telemetry every 10 seconds, and check if we are still connected
        if (telem_timer > 20000)
        {
            // Check connection
            shared.astrodev_tx.Ping(false);
            // shared.astrodev_tx.GetTCVConfig(false);
            // shared.astrodev_tx.GetTelemetry(false);
            telem_timer = 0;
            // Attempt receive of any of the above packets
            continue;
        }

        // Check if we are still connected
        if (last_connected > unconnected_timeout)
        {
            // Handle not-connnected status
            iretn = shared.connect_radio(shared.astrodev_tx, 400800, 400800);
            if (iretn < 0)
            {
                // Keep attempting reconnect
                continue;
            }
            last_connected = 0;
        }

        // Transmit bursts of 10 seconds
        while (shared.pop_queue(shared.send_queue, shared.send_lock, packet) >= 0
        && telem_timer < 10000)
        {
            send_tx_packet(packet);
        }
    }
}

void Cosmos::Module::Radio_interface::handle_tx_recv(const Astrodev::frame& msg)
{
    packet.header.nodedest = IOBC_NODE_ID;
    packet.header.nodeorig = IOBC_NODE_ID;

    // Handle tx radio responses
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
        packet.data[0] = LI3TX;
        packet.data[1] = 0x20;
        packet.data[2] = (uint8_t)msg.header.command;
        packet.data[3] = msg.header.sizelo;
        memcpy(packet.data.data()+4, &msg.payload[0], msg.header.sizelo);
        break;
    case Astrodev::Command::TRANSMIT:
            // Transmit ACK
            return;
        break;
    case Astrodev::Command::RECEIVE:
        // Discard any packets received on the tx radio
        return;
#else
    // These cases here are for faking a radio interaction
    // Any 
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
        exit(-1);
        break;
    }
    shared.push_queue(shared.main_queue, shared.main_lock, packet);
}

void Cosmos::Module::Radio_interface::send_tx_packet(Cosmos::Support::PacketComm& packet)
{
    int32_t iretn = 0;
    uint8_t retries = 0;
    Serial.println(" | Transmit message...");
    elapsedMillis ack_timeout;
    while(true)
    {
        ack_timeout = 0;
        if (!shared.astrodev_tx.buffer_full.load())
        {
            // Attempt transmit if transfer bull is not full
            iretn = shared.astrodev_tx.Transmit(packet);
            // Retry serial write error
            if (iretn < 0 && ((++retries) <= max_retries))
            {
                threads.delay(10);
                continue;
            }
            // Perform ACK check
            ack_timeout = 0;
            // while (!shared.astrodev_tx.ack_transmit.load() && ack_timeout < 1000)
            // {
            //     Serial.println("ACK CHECK");
            //     iretn = shared.astrodev_tx.Receive(incoming_message);
            //     if (iretn >= 0)
            //     {
            //         last_connected = 0;
            //         handle_tx_recv(incoming_message);
            //         break;
            //     }
            //     threads.delay(10);
            // }
            // Transmission attempt end

            // TODO: Perhaps attempt resend on NACK?
            break;
        }
        else
        {
            Serial.println("Buffer full, waiting a bit");
            // Retry 3 times
            if(++retries > 3)
            {
                break;
            }
            // Wait until transfer buffer is not full
            threads.delay(100);
            // Ping and attempt to get new radio availability state
            iretn = shared.astrodev_tx.Ping(true);
            if (iretn < 0)
            {
                // Ping error not handled
            }
            if (!shared.astrodev_tx.buffer_full.load())
            {
                Serial.println("buffer full flag cleared");
                threads.delay(10);
                // Return to start of loop to attempt transmit
                continue;
            }
        }
    };
}