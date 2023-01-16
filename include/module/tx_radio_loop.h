#ifndef _TX_RADIO_LOOP_H
#define _TX_RADIO_LOOP_H

#include "shared_resources.h"

namespace Cosmos
{
    namespace Module
    {
        namespace Radio_interface
        {
            // Combines send and recv into one loop,
            // based on the radio_loop() function in the
            // main astrodev repo
            void tx_radio_loop();

            // Transmit a packet pop'd from the send_queue
            void send_tx_packet(Cosmos::Support::PacketComm& packet);

            // Receive a packet from the tx radio
            void handle_tx_recv(const Cosmos::Devices::Radios::Astrodev::frame& msg);
        }
    }
}

#endif