
#include "module/radio_send.h"
#include "shared_resources.h"

// Debug values
int32_t sentNum = 0;
int32_t errSend = 0;

// Global shared defined in main.cpp
extern shared_resources shared;

namespace
{
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
    // Cosmos::Devices::Radios::Astrodev::frame response;
}

// TXS Loop
void Cosmos::Module::Radio_interface::txs_loop()
{
    Serial.println("txs_loop started");
    int32_t iretn = 0;

    // TXS loop continually attempts to flush its outgoing packet queue
    while(true)
    {
        // Send packet at front of queue
        iretn = shared.pop_queue(shared.send_queue, shared.send_lock, packet);
        if (iretn >= 0)
        {
            Cosmos::Module::Radio_interface::send_packet();
        } else {
            // Send queue was empty
            threads.delay(1000);
        }

        // Yield thread
        threads.delay(10);
        // ------- Stuff below here for debugging, remove later
        if (sentNum > 124)
        {
            Serial.print("Sent: ");
            Serial.println(sentNum);
            Serial.print("Errors: ");
            Serial.println(errSend);
            Serial.println("Returning from txs_loop");
            return;
        }
        Cosmos::Support::PacketComm p;
        p.data.resize(100);
        for (size_t i = 0; i < 100; ++i)
        {
            p.data[i] = i;
        }
        shared.push_queue(shared.send_queue, shared.send_lock, p);
        // ------- Debug end
    }
    return;
}

//! Send out a PacketComm packet with proper radio transmit buffer checks and retries
void Cosmos::Module::Radio_interface::send_packet()
{
    int32_t iretn = 0;
    uint8_t retries = 0;
    Serial.print(sentNum++);
    Serial.println(" | Transmit message...");
    // TODO: Attempt resend on NACK? Would be difficult to coordinate
    while(true)
    {
        if (!shared.astrodev.buffer_full.load())
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
            Serial.println("Buffer full, waiting a bit");
            // Retry 3 times
            if(++retries > 3)
            {
                break;
            }
            // Wait until transfer buffer is not full
            threads.delay(100);
            // Let recv_loop handle getting the response back and clearing buffer_full flag
            iretn = shared.astrodev.Ping(false);
            if (iretn < 0)
            {
                ++errSend;
            }
            if (!shared.astrodev.buffer_full.load())
            {
                Serial.println("buffer full flag cleared");
                threads.delay(10);
                // Return to start of loop to attempt transmit
                continue;
            }
        }
    };

    return;
}
