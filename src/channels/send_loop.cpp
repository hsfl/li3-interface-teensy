
#include "channels/send_loop.h"
#include "shared_resources.h"

int32_t sentNum = 0;
int32_t errSend = 0;

// Global shared defined in main.cpp
extern shared_resources shared;

// Reusable packet objects
namespace
{
    Cosmos::Support::PacketComm packet;
    Cosmos::Devices::Radios::Astrodev::frame response;
}

// TXS Loop
// Empties array
void Cosmos::Radio_interface::send_loop()
{
    int32_t iretn = 0;

    Serial.println("send_loop started");

    while(true)
    {
        // Testing message transmissions
        iretn = shared.pop_send(packet);
        if (iretn >= 0)
        {
            Serial.println("sending packet");
            Cosmos::Radio_interface::send_packet();
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
        Cosmos::Support::PacketComm p;
        p.data.resize(1,0);
        shared.push_send(p);
    }
    
    //threads.idle();
}

//! Send out a PacketComm packet with proper radio transmit buffer checks and retries
void Cosmos::Radio_interface::send_packet()
{
    uint8_t retries = 0;
    Serial.print(sentNum++);
    Serial.println(" | Transmit message...");
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
