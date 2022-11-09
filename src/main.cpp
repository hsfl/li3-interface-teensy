#include "shared_resources.h"
#include "module/radio_recv.h"
#include "module/radio_send.h"
#include "module/iobc_recv.h"

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
    // Setup serial stuff
    Serial.begin(115200);

    // Each thread tick length
    threads.setSliceMicros(10);
    // Wait for serial monitor
    Serial.println("Delay 4 sec");
    delay(4000);

    // Initialize the astrodev radio
    iretn = shared.init_radio(&Serial5, ASTRODEV_BAUD);
    if (iretn < 0)
    {
        Serial.println("Error initializing Astrodev radio. Exiting...");
        exit(-1);
    }
    Serial.println("Radio init successful");

    // TODO: determine more appropriate stack size
    // Start send/receive loops
    // threads.addThread(Cosmos::Module::Radio_interface::rxs_loop, 0, RXS_STACK_SIZE);
    threads.addThread(Cosmos::Module::Radio_interface::txs_loop, 0, TXS_STACK_SIZE);
    threads.addThread(Cosmos::Module::Radio_interface::iobc_recv_loop, 0, RXS_STACK_SIZE);

    Serial.println("Setup complete");

}

//! Main loop
void loop()
{
    // Process command-type packets for this program
    handle_main_queue_packets();

    // 

    // shared.astrodev.GetTelemetry();

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
        default:
            {
                Serial.println("Packet type not handled, exiting...");
                exit(-1);
            }
            break;
        }
    }
}
