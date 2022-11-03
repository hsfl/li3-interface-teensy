#include "shared_resources.h"
#include "channels/recv_loop.h"

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

char buf[256];
// RXS Loop
void Cosmos::Radio_interface::recv_loop()
{
    // int32_t iretn;
    packet.header.dest = 0;
    Serial.println("recv_loop started");
    // Wait for main loop to start
    delay(1000);
    while (true)
    {
        threads.delay(10);
    //     uint16_t size = 0;
    //     uint8_t buflen = shared.HWSerial->available();
    //     packet.wrapped.resize(256);
    //     char msg[16];
    //     sprintf(msg, "available: %03d", buflen);
    //     Serial.println(msg);
    //     while(!shared.SLIPHWSerial.endofPacket())
    //     {
    //     // len is either 0 (done) or 1 (not end)
    //     uint8_t len = shared.SLIPHWSerial.available();
    //     // If there are bytes in receive buffer to be read
    //     if (len > 0)
    //     {
    //         // Read a slip packet
    //         while(len--)
    //         {
    //         // Note: goes into wrapped because SLIPHWSerial.read()
    //         // already decodes SLIP encoding
    //         packet.wrapped[size] = shared.SLIPHWSerial.read();
    //         //buf[size] = SLIPHWSerial.read();
    //         size++;
    //         }
    //     }
    //     }
    //     packet.wrapped.resize(size);
    //     // iretn = packet.SLIPUnPacketize();
    //     iretn = packet.Unwrap();
    //     Serial.print("iretn: ");
    //     Serial.print(iretn);
    //     Serial.print(" size ");
    //     Serial.print(size);
    //     Serial.print(" wrapped.size ");
    //     Serial.print(packet.wrapped.size());
    //     Serial.print(" data.size ");
    //     Serial.println(packet.data.size());
    //     for (size_t i=0; i < packet.wrapped.size(); ++i) {
    //         Serial.print(packet.wrapped[i]);
    //         Serial.print(" ");
    //     }
    //     Serial.println();
    //     for (size_t i=0; i < packet.data.size(); ++i) {
    //         Serial.print(char(packet.data[i]));
    //     }
    //     Serial.println();

        iretn = shared.astrodev.Receive(incoming_message);
        if (iretn < 0)
        {
            // Yield until successful read
            threads.delay(10);
        }
        else
        {
            //packet.header.radio = ...; // Don't know atm

            packet.header.dest = IOBC_NODE_ID;
            packet.header.orig = GROUND_NODE_ID;

            // Handle payload
            Astrodev::Command cmd = (Astrodev::Command)iretn;
            switch (cmd)
            {
            // 0 is Acknowledge-type, view comment on return types for Astrodev::Receive()
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
            shared.push_recv(packet);
        }
    }
}