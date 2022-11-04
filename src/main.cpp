#include "shared_resources.h"
#include "module/radio_recv.h"
#include "module/radio_send.h"

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
    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    // Each thread tick length
    threads.setSliceMicros(10);
    // Wait for serial monitor
    Serial.println("Delay 5 sec");
    delay(5000);

    // Setup serial stuff
    Serial.begin(115200);

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
    threads.addThread(Cosmos::Module::Radio_interface::rxs_loop, 0, RXS_STACK_SIZE);
    threads.addThread(Cosmos::Module::Radio_interface::txs_loop, 0, TXS_STACK_SIZE);

    Serial.println("Setup complete");

}

//! Main loop
void loop()
{
    Serial.println("in main loop");
    /*
    // turn the LED on (HIGH is the voltage level)
    digitalWrite(LED_BUILTIN, HIGH);
    //Serial.println("sending!");
    packet.header.type = PacketComm::TypeId::CommandClearQueue;
    packet.header.orig = 1;
    packet.header.dest = 2;
    packet.header.radio = 3;
    packet.header.data_size = 12;
    char str[12] = "hello world";
    packet.data.insert(packet.data.end(), str, str + 12);
    iretnSend = packet.SLIPPacketize();
    if (iretnSend)
    {
        HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
    }
    packet.data.clear();
    // SLIPHWSerial.beginPacket();
    // SLIPHWSerial.print(str);
    // SLIPHWSerial.endPacket();
    // wait for a bit
    threads.delay(10);
    // turn the LED off by making the voltage LOW
    digitalWrite(LED_BUILTIN, LOW);
    //Serial.println("send sleeping...");
    */

    // Ping code
    // Serial.print(sentNum++);
    // Serial.print(" | Pinging...");
    // int32_t iretn = astrodev.Ping();
    // Serial.print(" iretn: ");
    // Serial.println(iretn);

    // Handle all packets in recv queue, process or forward to iobc
    // In this case, they are all forwarded
    // Recv->Iobc | (process)

    // Take everything from iobc and process or forward to tx queue
    // Iobc->tx | process

    // Send all received packets to iobc
    // iretn = shared.pop_recv(packet);
    // if (iretn >= 0)
    // {
    //     Serial.print("I have a packet, buf size: ");
    //     Serial.println(iretn);
    //     char msg[4];
    //     Serial.print("main: ");
    //     for (uint16_t i=0; i<packet.data.size(); i++)
    //     {
    //         sprintf(msg, "0x%02X", packet.data[i]);
    //         Serial.print(msg);
    //         Serial.print(" ");
    //     }
    //     Serial.println();
    // }

    // shared.astrodev.GetTelemetry();

    threads.delay(9000);
}
