#include "shared_resources.h"
#include "helpers/RadioCommands.h"
#include "module/radio_recv.h"
#include "module/radio_send.h"
#include "module/tx_radio_loop.h"
#include "module/iobc_recv.h"
#include "helpers/TestBlinker.h"

// For rebooting the teensy on software init failure
#define RESTART_ADDR       0xE000ED0C
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

// Function forward declarations
void sendpacket(Cosmos::Support::PacketComm &packet);
void handle_main_queue_packets();

//! Global shared resources defined here
shared_resources shared(Serial1);

namespace
{
    int32_t iretn = 0;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
}

void setup()
{
    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // Setup serial stuff
    Serial.begin(115200);

    // Each thread tick length
    threads.setSliceMicros(10);

    // BURN WIRE CODE DON'T TOUCH
    // Must be HIGH for about 30 seconds
    // Serial.println("Enabling burn wire");
    // pinMode(12, OUTPUT);
    // digitalWrite(12, HIGH);
    // delay(30000);
    // digitalWrite(12, LOW);

    // Initialize the astrodev radio
    iretn = shared.init_radios(&Serial5, &Serial2, ASTRODEV_BAUD);
    if (iretn < 0)
    {
        Serial.println("Error initializing Astrodev radio. Restarting...");
        delay(1000);
        WRITE_RESTART(0x5FA0004);
        exit(-1);
    }
    Serial.println("Radio init successful");

    // TODO: determine more appropriate stack size
    // Start send/receive loops
    threads.addThread(Cosmos::Module::Radio_interface::rx_recv_loop, 0, RX_STACK_SIZE);
    // threads.addThread(Cosmos::Module::Radio_interface::tx_recv_loop, 0, RX_STACK_SIZE);
    // threads.addThread(Cosmos::Module::Radio_interface::send_loop, 0, TX_STACK_SIZE);
    threads.addThread(Cosmos::Module::Radio_interface::tx_radio_loop, 0, TX_STACK_SIZE);
    threads.addThread(Cosmos::Module::Radio_interface::iobc_recv_loop, 0, RX_STACK_SIZE);

    Serial.println("Setup complete");

}

//! Main loop
void loop()
{
    // Process command-type packets for this program
    handle_main_queue_packets();

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
        case PacketComm::TypeId::CommandRadioAstrodevCommunicate:
            {
                // These are our periodic telem grabbing responses, send to iobc
                if (packet.header.nodeorig == IOBC_NODE_ID)
                {
                    Serial.print("Got radio communicate response unit:");
                    Serial.print(packet.data[0]);
                    Serial.print(" type:");
                    Serial.print(packet.data[2]);
                    Serial.println(", forwarding to iobc");
                    iretn = packet.Wrap();
                    if (iretn < 0)
                    {
                        Serial.println("Error in Wrap()");
                    }
                    shared.SLIPIobcSerial.beginPacket();
                    shared.SLIPIobcSerial.write(packet.wrapped.data(), packet.wrapped.size());
                    shared.SLIPIobcSerial.endPacket();
                }
                // If the packet command came from ground, execute
                else if (packet.header.nodeorig == GROUND_NODE_ID)
                {
                    Lithium3::RadioCommand(packet);
                }
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
                Serial.print("Packet type ");
                Serial.print((uint16_t)packet.header.type);
                Serial.println(" not handled, forwarding to iobc");
                iretn = packet.Wrap();
                if (iretn < 0)
                {
                    Serial.println("Error in Wrap()");
                }
                shared.SLIPIobcSerial.beginPacket();
                shared.SLIPIobcSerial.write(packet.wrapped.data(), packet.wrapped.size());
                shared.SLIPIobcSerial.endPacket();
            }
            break;
        }
    }
}
