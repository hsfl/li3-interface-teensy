#include "helpers/RadioCommands.h"

// Global shared defined in main.cpp
extern shared_resources shared;

void Reboot()
{
    Serial.println("Got Reboot command. Restarting...");
    delay(1000);
    WRITE_RESTART(0x5FA0004);
}

// Args:
// Byte 0 = Unit (doesn't matter)
// Byte 1 = doesn't matter
// Byte 2 = 254
// Byte 3-4 = Number of response bytes (0)
// Byte 5 = HIGH (1) or LO (0)
void SetBurnwire(const Cosmos::Support::PacketComm &packet)
{
    if (packet.data.size() < 5 || packet.data[4] > 1)
    {
        Serial.println("Burn wire command incorrect args.");
        return;
    }
    // BURN WIRE CODE DON'T TOUCH
    // Must be HIGH for about 30 seconds
    Serial.println("Enabling burn wire");
    pinMode(12, OUTPUT);
    digitalWrite(12, packet.data[4]);
}

void Lithium3::RadioCommand(Cosmos::Support::PacketComm &packet)
{
    // Args:
    // Byte 0 = Unit
    // Byte 1-2 = Command
    // Bytes 3-4 = Number of response bytes
    // Byte 5 - (N-1) = Bytes
    //
    // Unit:
    // - 0 is rx radio, 1 is tx radio

    switch(packet.data[2])
    {
    case 5:
        !packet.data[0] ? shared.astrodev_rx.GetTCVConfig(false) : shared.astrodev_tx.GetTCVConfig(false);
        break;
    case 7:
        !packet.data[0] ? shared.astrodev_rx.GetTelemetry(false) : shared.astrodev_tx.GetTelemetry(false);
        break;
    case 254:
        SetBurnwire(packet);
        break;
    case 255:
        Reboot();
        break;
    default:
        break;
    }
}