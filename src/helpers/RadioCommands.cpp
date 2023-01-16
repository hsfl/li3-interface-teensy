#include "helpers/RadioCommands.h"

// Global shared defined in main.cpp
extern shared_resources shared;

void Lithium3::RadioCommand(Cosmos::Support::PacketComm &packet)
{
    switch(packet.data[1])
    {
    case 7:
        shared.astrodev_tx.GetTelemetry(false);
        break;
    default:
        break;
    }
}