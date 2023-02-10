#include "helpers/RadioCommands.h"

// Global shared defined in main.cpp
extern shared_resources shared;

void Lithium3::RadioCommand(Cosmos::Support::PacketComm &packet)
{
    switch(packet.data[2])
    {
    case 5:
        !packet.data[0] ? shared.astrodev_rx.GetTCVConfig(false) : shared.astrodev_tx.GetTCVConfig(false);
        break;
    case 7:
        !packet.data[0] ? shared.astrodev_rx.GetTelemetry(false) : shared.astrodev_tx.GetTelemetry(false);
        break;
    default:
        break;
    }
}