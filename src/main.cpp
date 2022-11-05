#include "shared_resources.h"
#include "module/radio_recv.h"
#include "module/radio_send.h"
#include "module/iobc_recv.h"

// Function forward declarations
void sendpacket(Cosmos::Support::PacketComm &packet);

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
    //threads.addThread(Cosmos::Module::Radio_interface::rxs_loop, 0, RXS_STACK_SIZE);
    //threads.addThread(Cosmos::Module::Radio_interface::txs_loop, 0, TXS_STACK_SIZE);
    threads.addThread(Cosmos::Module::Radio_interface::iobc_recv_loop, 0, RXS_STACK_SIZE);

    Serial.println("Setup complete");

}

//! Main loop
void loop()
{
    Serial.println("in main loop");

    // Handle all packets in recv queue, process or forward to iobc
    // In this case, they are all forwarded
    // Recv->Iobc | (process)

    // Take everything from iobc and process or forward to tx queue
    // Iobc->tx | process

    // Handle all packets in recv queue, process or forward to iobc
    // In this case, they are all forwarded
    // Recv->Iobc | (process)

    // Take everything from iobc and process or forward to tx queue
    // Iobc->tx | process

    // Send all received packets to iobc
    iretn = shared.pop_queue(shared.main_queue, shared.main_lock, packet);
    if (iretn >= 0)
    {
        Serial.print("I have a packet, buf size: ");
        Serial.println(iretn);
        char msg[4];
        Serial.print("main: ");
        for (uint16_t i=0; i<packet.data.size(); i++)
        {
            sprintf(msg, "0x%02X", packet.data[i]);
            Serial.print(msg);
            Serial.print(" ");
        }
        Serial.println();
    }

    // shared.astrodev.GetTelemetry();

    threads.delay(9000);
}
