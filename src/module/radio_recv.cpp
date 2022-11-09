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
}

// RXS Loop
void Cosmos::Module::Radio_interface::rxs_loop()
{
    // int32_t iretn;
    packet.header.dest = 0;
    Serial.println("rxs_loop started");
    
    while (true)
    {
        threads.delay(10);

        iretn = shared.astrodev.Receive(incoming_message);
        if (iretn < 0)
        {
            // Yield until successful read
            continue;
        }
        else
        {
            //packet.header.radio = ...; // TODO: Don't know atm

            packet.header.dest = IOBC_NODE_ID;
            packet.header.orig = GROUND_NODE_ID;

            // Handle payload
            Astrodev::Command cmd = (Astrodev::Command)iretn;
            switch (cmd)
            {
            // 0 is Acknowledge-type, view comment on return types for Astrodev::Receive()
            // Though this does mean that I can't distinguish between acks and noacks
            case (Astrodev::Command)0:
                continue;
            case Astrodev::Command::GETTCVCONFIG:
            case Astrodev::Command::TELEMETRY:
                // Setup PacketComm packet stuff
                packet.header.type = Cosmos::Support::PacketComm::TypeId::DataRadioResponse;
                packet.data.resize(incoming_message.header.sizelo + 1);
                packet.data[0] = (uint8_t)cmd;
                memcpy(packet.data.data(), &incoming_message.payload[0], incoming_message.header.sizelo);
                break;
            case Astrodev::Command::RECEIVE:
                // Packets from the ground will be in PacketComm protocol
                // TODO: get rid of redundant unwrap/wrap that will probably be happening at sending this back to iobc
                Serial.println("in receive");
                packet.wrapped.resize(incoming_message.header.sizelo);
                memcpy(packet.wrapped.data(), &incoming_message.payload[0], incoming_message.header.sizelo);
                iretn = packet.Unwrap();
                if (iretn < 0)
                {
                    continue;
                }
                break;
            default:
                Serial.print("cmd ");
                Serial.print((uint16_t)cmd);
                Serial.println("not (yet) handled. Terminating...");
                exit(-1);
                break;
            }
            Serial.print("pushing to main queue, cmd ");
            Serial.println((uint16_t)cmd);
            shared.push_queue(shared.main_queue, shared.main_lock, packet);
        }
    }
}