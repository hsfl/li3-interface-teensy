#ifndef _TXS_LOOP_H
#define _TXS_LOOP_H

#include "shared_resources.h"

namespace Cosmos
{
    namespace Module
    {
        namespace Radio_interface
        {
            void send_packet();
            void txs_loop();
        }
    }
}

#endif