#include "helpers/RadioCommands.h"

// Global shared defined in main.cpp
extern shared_resources shared;

// Args:
// Byte 0 = Unit (doesn't matter)
// Byte 1 = doesn't matter
// Byte 2 = 255
// Byte 3 = Number of response bytes (0)
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
// Byte 3 = Number of response bytes (0)
// Byte 4 = Attempt ID
// Byte 5 = HIGH (1) or LOW (0)
// Byte 6 = Time (s) to keep on (Optional, default 0)
void SetBurnwire(const Cosmos::Support::PacketComm &packet)
{
    if (packet.data.size() < 6)
    {
        Serial.println("Burn wire command incorrect number of args");
        return;
    }

    // Burn wire pin will be set LOW after burnwire_timer exceeds this value
    // Default of 0 will keep it on until manually commanded to be set LOW
    shared.burnwire_on_time = 0;
    if (packet.data.size() == 7 && shared.burnwire_state)
    {
        shared.burnwire_on_time = packet.data[6] * 1000;
    }

    // To be sent back to the iobc the burn ACK is created
    shared.attempt_id = packet.data[4];

    if (packet.data[5] > 1)
    {
        Serial.println("Burn wire state arg must be 1 (HIGH) or 0 (LOW)!");
        return;
    }
    shared.burnwire_state = packet.data[5] ? HIGH : LOW;

    Serial.print("Setting burn wire ");
    Serial.print(shared.burnwire_state ? "HIGH" : "LOW");
    if (shared.burnwire_on_time)
    {
        Serial.print(" for ");
        Serial.print(unsigned(packet.data[6]) * 1000);
        Serial.print(" milliseconds.");
    }
    Serial.println();
    Serial.println("BURNWIRE CODE IS CURRENTLY COMMENTED OUT!"); // Remember to comment back in main.cpp too!
    // pinMode(12, OUTPUT);
    // digitalWrite(12, shared.burnwire_state);

    // Reset timer to keep burnwire set HIGH
    shared.burnwire_timer = 0;
}

void Lithium3::RadioCommand(Cosmos::Support::PacketComm &packet)
{
    int32_t iretn = 0;

    // Args:
    // Byte 0 = Unit
    // Byte 1-2 = Command
    // Bytes 3 = Number of response bytes
    // Byte 4 - (N-1) = Bytes
    //
    // Unit:
    // - 0 is rx radio, 1 is tx radio
    if (packet.data.size() < 4)
    {
        return;
    }

    using namespace Cosmos::Devices::Radios;

    // For regular UHF commands, don't run them if the respective radio
    // is not yet initialized
    if (packet.data[2] <= (unsigned)Astrodev::Command::ALARM_RTC)
    {
        if (!packet.data[0] && !shared.get_rx_radio_initialized_state())
        {
            return;
        }
        if (packet.data[0] && !shared.get_tx_radio_initialized_state())
        {
            return;
        }
    }



    switch (packet.data[2])
    {
    case 0x05: // Get TCV Config
        !packet.data[0] ? shared.astrodev_rx.GetTCVConfig(false) : shared.astrodev_tx.GetTCVConfig(false);
        break;
    case 0x07: // Get Telemetry
        !packet.data[0] ? shared.astrodev_rx.GetTelemetry(false) : shared.astrodev_tx.GetTelemetry(false);
        break;
    case 0x06: // Set TCV Config
        {
            Cosmos::Devices::Radios::Astrodev *astrodev;
            !packet.data[0] ? astrodev = &shared.astrodev_rx : astrodev = &shared.astrodev_tx;

            if (packet.data.size() < 11)
            {
                return;
            }


            Cosmos::Devices::Radios::Astrodev::tcv_config new_tcv_config;
            memcpy(&new_tcv_config, &packet.data[4], sizeof(new_tcv_config));

            Serial.print("Setting power level to: ");
            Serial.println(unsigned(new_tcv_config.power_amp_level));

            if (new_tcv_config.power_amp_level == astrodev->tcv_configuration.power_amp_level)
            {
                Serial.println("Power level is already the requested value.");
                return;
            }

            // Assuming here that only the tx radio will have its settings changed
            Threads::Scope lock(shared.tx_lock);

            astrodev->tcv_configuration = new_tcv_config;

            int8_t retries = RADIO_INIT_CONNECT_ATTEMPTS;
            while(true)
            {
                while ((iretn = astrodev->SetTCVConfig()) < 0 && --retries > 0)
                {
                    Serial.println("Resetting");
                    astrodev->Reset();
                    Serial.println("Failed to settcvconfig astrodev");
                    threads.delay(5000);
                }
                if (iretn >= 0)
                {
                    Serial.println("SetTCVConfig successful");
                }

                while ((iretn = astrodev->GetTCVConfig()) < 0 && --retries > 0)
                {
                    Serial.println("Failed to gettcvconfig astrodev");
                    threads.delay(5000);
                }
                Serial.print("Checking config settings... ");
                if (astrodev->tcv_configuration.interface_baud_rate != new_tcv_config.interface_baud_rate
                    || astrodev->tcv_configuration.power_amp_level  != new_tcv_config.power_amp_level
                    || astrodev->tcv_configuration.rx_baud_rate     != new_tcv_config.rx_baud_rate
                    || astrodev->tcv_configuration.tx_baud_rate     != new_tcv_config.tx_baud_rate
                    || astrodev->tcv_configuration.ax25_preamble_length  != new_tcv_config.ax25_preamble_length
                    || astrodev->tcv_configuration.ax25_postamble_length != new_tcv_config.ax25_postamble_length)
                {
                    Serial.println("config mismatch detected!");
                    Serial.print("interface_baud_rate: ");
                    Serial.print(unsigned(astrodev->tcv_configuration.interface_baud_rate));
                    Serial.print(" | ");
                    Serial.println(unsigned(new_tcv_config.interface_baud_rate));
                    Serial.print("power_amp_level: ");
                    Serial.print(unsigned(astrodev->tcv_configuration.power_amp_level));
                    Serial.print(" | ");
                    Serial.println(unsigned(new_tcv_config.power_amp_level));
                    Serial.print("rx_baud_rate: ");
                    Serial.print(unsigned(astrodev->tcv_configuration.rx_baud_rate));
                    Serial.print(" | ");
                    Serial.println(unsigned(new_tcv_config.rx_baud_rate));
                    Serial.print("tx_baud_rate: ");
                    Serial.print(unsigned(astrodev->tcv_configuration.tx_baud_rate));
                    Serial.print(" | ");
                    Serial.println(unsigned(new_tcv_config.tx_baud_rate));
                    Serial.print("ax25_preamble_length: ");
                    Serial.print(unsigned(astrodev->tcv_configuration.ax25_preamble_length));
                    Serial.print(" | ");
                    Serial.println(unsigned(new_tcv_config.ax25_preamble_length));
                    Serial.print("ax25_postamble_length: ");
                    Serial.print(unsigned(astrodev->tcv_configuration.ax25_postamble_length));
                    Serial.print(" | ");
                    Serial.println(unsigned(new_tcv_config.ax25_postamble_length));
                    --retries;
                }
                else
                {
                    break;
                }
                if (retries < 0)
                {
                    // Reboot if encountering excessive difficulty
                    WRITE_RESTART(0x5FA0004);
                }
            }
            Serial.println("config check OK!");
        }
        break;
    case 0x20: // Fast PA Set
        {
            Cosmos::Devices::Radios::Astrodev *astrodev;
            !packet.data[0] ? astrodev = &shared.astrodev_rx : astrodev = &shared.astrodev_tx;

            if (packet.data.size() < 5)
            {
                return;
            }

            Serial.print("Setting power level to: ");
            Serial.println(unsigned(packet.data[4]));

            if (packet.data[4] == astrodev->tcv_configuration.power_amp_level)
            {
                Serial.println("Power level is already the requested value.");
                return;
            }

            // Assuming here that only the tx radio will have its settings changed
            Threads::Scope lock(shared.tx_lock);

            astrodev->tcv_configuration.power_amp_level = packet.data[4];

            int8_t retries = RADIO_INIT_CONNECT_ATTEMPTS;

            while ((iretn = astrodev->SetPowerAmpFast()) < 0 && --retries > 0)
            {
                threads.delay(2000);
            }
            if (iretn < 0 && retries < 0)
            {
                // Reboot if encountering excessive difficulty
                WRITE_RESTART(0x5FA0004);
            }
            Serial.println("SetPowerAmpFast successful");
        }
        break;
    case CMD_BURNWIRE:
        SetBurnwire(packet);
        break;
    case CMD_REBOOT:
        Reboot();
        break;
    default:
        break;
    }
}