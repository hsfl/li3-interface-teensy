
#include "channels/send_loop.h"
#include "shared_resources.h"

// Debug values
int32_t sentNum = 0;
int32_t errSend = 0;

// Global shared defined in main.cpp
extern shared_resources shared;

namespace
{
    int32_t iretn = 0;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
    Cosmos::Devices::Radios::Astrodev::frame response;
}

// TXS Loop
void Cosmos::Radio_interface::send_loop()
{
    Serial.println("send_loop started");

    // TXS loop continually attempts to flush its outgoing packet queue
    while(true)
    {
        // Send packet at front of queue
        iretn = shared.pop_send(packet);
        if (iretn >= 0)
        {
            Cosmos::Radio_interface::send_packet();
        } else {
            // Send queue was empty
            threads.delay(1000);
        }

        // Yield thread
        threads.delay(10);
        // ------- Stuff below here for debugging, remove later
        if (sentNum > 24)
        {
            Serial.print("Sent: ");
            Serial.println(sentNum);
            Serial.print("Errors: ");
            Serial.println(errSend);
            Serial.println("Returning from send_loop");
            return;
        }
        Cosmos::Support::PacketComm p;
        p.data.resize(1,0);
        shared.push_send(p);
        // ------- Debug end
    }
    return;
}

//! Send out a PacketComm packet with proper radio transmit buffer checks and retries
void Cosmos::Radio_interface::send_packet()
{
    uint8_t retries = 0;
    Serial.print(sentNum++);
    Serial.println(" | Transmit message...");
    // TODO: Attempt resend on NACK? Would be difficult to coordinate
    while(true)
    {
        if (!shared.astrodev.buffer_full)
        {
            // Attempt transmit if transfer bull is not full
            iretn = shared.astrodev.Transmit(packet);
            Serial.print("Transmit iretn: ");
            Serial.println(iretn);
            if (iretn < 0)
            {
                ++errSend;
            }
            // Transmit successful
            break;
        }
        else
        {
            // Retry 3 times
            if(++retries > 3)
            {
                break;
            }
            // Wait until transfer buffer is not full
            threads.delay(100);
            iretn = shared.astrodev.Ping();
            if (iretn < 0)
            {
                ++errSend;
            }
            if (!shared.astrodev.buffer_full)
            {
                threads.delay(10);
                // Return to start of loop to attempt transmit
                continue;
            }
        }
    };

    return;
}
