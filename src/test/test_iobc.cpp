#include "shared_resources.h"
#include "module/iobc_recv.h"

#define HWSERIAL Serial1

//! Global shared resources defined here
shared_resources shared(Serial1);

void handle_main_queue_packets();

namespace
{
    int32_t iretn = 0;
    Cosmos::Support::PacketComm packet;
    uint32_t send_counter = 0;
    elapsedMillis telem_timer;
    elapsedMillis transmit_timer;
}

void setup()
{
    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // Setup serial stuff
    Serial.begin(115200);

    // Each thread tick length
    threads.setSliceMicros(10);

    threads.addThread(Cosmos::Module::Radio_interface::iobc_recv_loop, 0, RX_STACK_SIZE);
    
    threads.delay(2000);
}

void send_get_telem_packet()
{
    packet.header.type = PacketComm::TypeId::CommandRadioCommunicate;
    packet.header.orig = IOBC_NODE_ID;
    packet.header.dest = IOBC_NODE_ID;
    packet.header.radio = 0;
    packet.data.resize(4);
    // Get rx telem
    packet.data[0] = LI3RX;
    packet.data[1] = 7;
    packet.data[2] = 0;
    packet.data[3] = 0;
    iretn = packet.SLIPPacketize();
    if (iretn)
    {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        threads.delay(10);
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        threads.delay(10);
    }
    // Get tx telem
    packet.data[0] = LI3TX;
    iretn = packet.SLIPPacketize();
    if (iretn)
    {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        threads.delay(10);
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        threads.delay(10);
    }
    packet.data.clear();
    // wait for a bit
    threads.delay(10);
}

void send_test_transmit_packet()
{
    packet.header.type = PacketComm::TypeId::CommandPing;
    packet.header.orig = IOBC_NODE_ID;
    packet.header.dest = GROUND_NODE_ID;
    packet.header.radio = 0;
    const size_t REPEAT = 124;
    // ascend from 0 to REPEAT-1, then last 4 bytes is packet number
    packet.data.resize(REPEAT + sizeof(send_counter));
    for (size_t i = 0; i < REPEAT; ++i)
    {
        packet.data[i] = i & 0xFF;
    }
    memcpy(packet.data.data()+REPEAT, &send_counter, sizeof(send_counter));
    iretn = packet.SLIPPacketize();
    if (iretn)
    {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        threads.delay(10);
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
#ifdef DEBUG_PRINT
        Serial.print("wrapped.size:");
        Serial.print(packet.wrapped.size());
        Serial.print(" packetized.size:");
        Serial.print(packet.packetized.size());
        Serial.print(" write size: ");
        Serial.println(HWSERIAL.availableForWrite());
        char msg[4];
        Serial.print("wrapped: ");
        for (uint16_t i=0; i<packet.wrapped.size(); i++)
        {
            sprintf(msg, "0x%02X", packet.wrapped[i]);
            Serial.print(msg);
            Serial.print(" ");
        }
        Serial.println();
#endif
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        threads.delay(10);
    }
    packet.data.clear();
    ++send_counter;
    // wait for a bit
    threads.delay(10);
}

void loop()
{
    // Process command-type packets for this program
    handle_main_queue_packets();

    // Get telem every 5 seconds
    if (telem_timer > 5000)
    {
        telem_timer -= 5000;
        send_get_telem_packet();
    }

    // Send transmit packets every second
    if (transmit_timer > 1000)
    {
        transmit_timer -= 1000;
        send_test_transmit_packet();
    }
    
    threads.delay(10);
}

// Any packets inside the main queue are command-type packets
// intended for this program to process in some way.
void handle_main_queue_packets()
{
    // Send all received packets to iobc
    iretn = shared.pop_queue(shared.main_queue, shared.main_lock, packet);
    if (iretn >= 0)
    {
#ifdef DEBUG_PRINT
        char msg[4];
        Serial.print("main: ");
        for (uint16_t i=0; i<packet.data.size(); i++)
        {
            sprintf(msg, "0x%02X", packet.data[i]);
            Serial.print(msg);
            Serial.print(" ");
        }
        Serial.println();
#endif
        using namespace Cosmos::Support;
        switch(packet.header.type)
        {
        case PacketComm::TypeId::CommandPing:
            {
                Serial.println("Pong!");
            }
            break;
        case PacketComm::TypeId::DataRadioResponse:
            {
                Serial.print("Radio response, cmd: ");
                Serial.println(packet.data[0]);
            }
            break;
        default:
            {
                Serial.println("Packet type not handled. Exiting...");
                exit(-1);
            }
            break;
        }
    }
}
