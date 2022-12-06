#ifndef _RADIO_RECV_H
#define _RADIO_RECV_H

#include "shared_resources.h"

namespace Cosmos
{
    namespace Module
    {
        namespace Radio_interface
        {
            //! Listens for responses from the RX radio, as well as packets from ground
            void rx_recv_loop();
            //! Listens for responses from the TX radio
            void tx_recv_loop();
        }
    }
};

#endif