#ifndef _RADIO_SEND_H
#define _RADIO_SEND_H

#include "shared_resources.h"

namespace Cosmos
{
    namespace Module
    {
        namespace Radio_interface
        {
            int32_t send_packet(const PacketComm& packet);
            void send_loop();
        }
    }
}

#endif