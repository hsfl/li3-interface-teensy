#include "shared_resources.h"
#include "channels/recv_loop.h"
#include "channels/send_loop.h"

// Function forward declarations
void sendpacket(Cosmos::Support::PacketComm &packet);

//! Global shared resources defined here
shared_resources shared(Serial5);

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

    //shared = shared_resources(Serial5);
    int32_t iretn = shared.init_radio(&Serial8, 9600);
    if (iretn < 0)
    {
        Serial.println("Error initializing Astrodev radio. Exiting...");
        exit(-1);
    }
    Serial.println("Radio init successful");

    /*
    // Start recv loop
    //threads.addThread(recv_loop);
    */
    // TODO: determine more appropriate stack size
    threads.addThread(Cosmos::Radio_interface::send_loop, 0, 6000);

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

    threads.delay(5000);
}
