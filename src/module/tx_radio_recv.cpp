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

    elapsedMillis tx_telem_timer;
}

void Cosmos::Module::Radio_interface::tx_recv_loop()
{
    // int32_t iretn;
    packet.header.nodedest = 0;
    Serial.println("tx_recv_loop started");
    
    while (true)
    {
        threads.delay(10);

        if (tx_telem_timer > 20000)
        {
            // Check connection
            shared.astrodev_tx.Ping(false);
            // shared.astrodev_tx.GetTCVConfig(false);
            shared.astrodev_tx.GetTelemetry(false);
            tx_telem_timer = 0;
            // Attempt receive of any of the above packets
        }

        iretn = shared.astrodev_tx.Receive(incoming_message);
        if (iretn < 0)
        {
            // Serial.print("tx.Receive returned ");
            // Serial.println(iretn);
            // Yield until successful read
            continue;
        }
        else
        {
            // Serial.print("tx.Receive returned ");
            // Serial.print(iretn);
            // Serial.print(" [4]: ");
            // Serial.print(incoming_message.preamble[4]);
            // Serial.print(" [5]: ");
            // Serial.println(incoming_message.preamble[4]);
            //packet.header.radio = ...; // TODO: Don't know atm
            packet.header.nodedest = IOBC_NODE_ID;
            packet.header.nodeorig = IOBC_NODE_ID;

            // Handle payload
            Astrodev::Command cmd = (Astrodev::Command)iretn;
            switch (cmd)
            {
            // 0 is Acknowledge-type, view comment on return types for Astrodev::Receive()
            // Though this does mean that I can't distinguish between acks and noacks
            case (Astrodev::Command)0:
                continue;
#ifndef MOCK_TESTING
            case Astrodev::Command::NOOP:
            case Astrodev::Command::GETTCVCONFIG:
            case Astrodev::Command::TELEMETRY:
                // Setup PacketComm packet stuff
                packet.header.type = Cosmos::Support::PacketComm::TypeId::CommandRadioAstrodevCommunicate;
                packet.header.nodeorig = IOBC_NODE_ID;
                packet.header.nodedest = IOBC_NODE_ID;
                packet.data.resize(incoming_message.header.sizelo + 4);
                packet.data[0] = LI3TX;
                packet.data[1] = 0x20;
                packet.data[2] = (uint8_t)cmd;
                packet.data[3] = incoming_message.header.sizelo;
                memcpy(packet.data.data()+4, &incoming_message.payload[0], incoming_message.header.sizelo);
                break;
            case Astrodev::Command::TRANSMIT:
                    // Transmit ACK
                    continue;
                break;
            case Astrodev::Command::RECEIVE:
                // Packets from the ground will be in PacketComm protocol
                // TODO: get rid of redundant unwrap/wrap that will probably be happening at sending this back to iobc
                Serial.print("in tx receive, bytes:");
                Serial.println(incoming_message.header.sizelo);
                memcpy(packet.wrapped.data(), &incoming_message.payload[0], incoming_message.header.sizelo);
                iretn = packet.Unwrap();
                if (iretn < 0)
                {
                    Serial.println("Unwrap fail in TX radio recv");
                    continue;
                }
                break;
#else
            // These cases here are for faking a radio interaction
            // Any 
            case Astrodev::Command::RESET:
            case Astrodev::Command::NOOP:
            case Astrodev::Command::SETTCVCONFIG:
            case Astrodev::Command::GETTCVCONFIG:
                packet.header.type = Cosmos::Support::PacketComm::TypeId::DataRadioResponse;
                packet.data.resize(1);
                packet.data[0] = (uint8_t)cmd;
                break;
            
#endif
            default:
                Serial.print("cmd ");
                Serial.print((uint16_t)cmd);
                Serial.println(" not (yet) handled. Terminating...");
                exit(-1);
                break;
            }
#ifdef DEBUG_PRINT
            Serial.print("pushing to main queue, cmd ");
            Serial.println((uint16_t)cmd);
#endif
            shared.push_queue(shared.main_queue, shared.main_lock, packet);
        }
    }
}
