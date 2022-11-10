#ifndef _RADIO_RECV_H
#define _RADIO_RECV_H

#include "shared_resources.h"

namespace Cosmos
{
    namespace Module
    {
        namespace Radio_interface
        {
            //! Listens for responses from the RXS radio, as well as packets from ground
            void rxs_recv_loop();
            //! Listens for responses from the TXS radio
            void txs_recv_loop();
        }
    }
};

#endif