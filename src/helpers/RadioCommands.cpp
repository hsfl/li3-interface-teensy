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
// Byte 5 = HIGH (1) or LOW (0)
// Byte 6 = Time (s) to keep on (Optional, default 0)
void SetBurnwire(const Cosmos::Support::PacketComm &packet)
{
    if (packet.data.size() < 5)
    {
        Serial.println("Burn wire command incorrect number of args");
        return;
    }

    // burn wire pin will be set LOW after burnwire_timer exceeds this value
    shared.burnwire_on_time = 0; 
    if (packet.data.size() == 6 && shared.burnwire_state)
    {
        shared.burnwire_on_time = packet.data[5] * 1000;
    }

    shared.burnwire_state = packet.data[4] ? HIGH : LOW;

    Serial.print("Setting burn wire ");
    Serial.print(shared.burnwire_state ? "HIGH" : "LOW");
    if (shared.burnwire_on_time)
    {
        Serial.print(" for ");
        Serial.print(unsigned(packet.data[5]) * 1000);
        Serial.print(" milliseconds.");
    }
    Serial.println();
    pinMode(12, OUTPUT);
    digitalWrite(12, shared.burnwire_state);

    // Reset timer to keep burnwire set HIGH
    shared.burnwire_timer = 0;
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
    if (packet.data.size() < 3)
    {
        return;
    }

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