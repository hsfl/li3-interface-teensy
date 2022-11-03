/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

// #include <Arduino.h>
// #include <map>
// #include <TeensyThreads.h>
// #include <SLIPEncodedSerial.h>

// #include <iterator>
#include "shared_resources.h"
// #include "math/bytelib.h"
// #include "math/crclib.h"

// Function forward declarations
void sendpacket(Cosmos::Support::PacketComm &packet);

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

    /*
    SLIPHWSerial.begin(115200);
    Serial.clear();
    HWSERIAL.clear();

    // Start recv loop
    //threads.addThread(recv_loop);
    */
}

PacketComm tpacket;
int32_t iretnSend = 0;

int32_t sentNum = 0;
int32_t errSend = 0;
// TXS Loop
// Empties array
void loop()
{
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

    // Testing message transmissions
    iretnSend = shared.pop_send(tpacket);
    if (iretnSend >= 0)
    {
        sendpacket(tpacket);
    } else {
        Serial.println("send buf empty");
        threads.delay(1000);
    }

    // wait for 3 seconds
    threads.delay(10);
    if (sentNum > 100)
    {
        Serial.print("Sent: ");
        Serial.println(sentNum);
        Serial.print("Errors: ");
        Serial.println(errSend);
        exit(-1);
    }
    //threads.idle();
}

void sendpacket(Cosmos::Support::PacketComm &packet)
{
    uint8_t retries = 0;
    Serial.print(sentNum++);
    Serial.println(" | Transmit message...");
    Cosmos::Devices::Radios::Astrodev::frame response;
    int32_t iretn = 0;
    // Attempt resend on NACK
    while(true)
    {
        iretn = shared.astrodev.Transmit(packet);
        Serial.print("Transmit iretn: ");
        Serial.println(iretn);
        if (iretn < 0)
        {
            ++errSend;
        break;
        }
        iretn = shared.astrodev.Receive(response);
        Serial.print("Recieve iretn: ");
        Serial.println(iretn);
        // Wait abit on NACK or if the transmit buffer is full
        if (iretn == COSMOS_ASTRODEV_ERROR_NACK || shared.astrodev.buffer_full)
        {
            Serial.print("iretn: ");
            Serial.print(iretn);
            Serial.print(" buf: ");
            Serial.println(shared.astrodev.buffer_full);
            threads.delay(1000);
        }
        // All other errors are actual errors
        else if (iretn < 0)
        {
            ++errSend;
            break;
        }
        // Transmit success
        if(iretn >= 0)
        {
            break;
        }
        // Retry 3 times
        if(retries++ > 3)
        {
            ++errSend;
            break;
        }
    };

    return;
}

