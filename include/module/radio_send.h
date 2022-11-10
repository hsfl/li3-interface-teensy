#ifndef _RADIO_SEND_H
#define _RADIO_SEND_H

#include "shared_resources.h"

namespace Cosmos
{
    namespace Module
    {
        namespace Radio_interface
        {
            void send_packet();
            void send_loop();
        }
    }
}

#endif