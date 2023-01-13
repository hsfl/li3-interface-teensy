#include "shared_resources.h"
#include "module/radio_recv.h"

using namespace Cosmos::Devices::Radios;

// UART port to RX recv port on li3 switcher board
#define HWSERIAL Serial2

//! Global shared resources defined here
shared_resources shared(Serial1);

void send_test_transmit_packet();
void handle_main_queue_packets();
void fake_radio_response_creator(Astrodev::Command cmd);
int32_t fake_transmit(Astrodev::frame &message, uint8_t size);
int32_t fake_transmit(PacketComm& packet);


namespace
{
    int32_t iretn = 0;
    Cosmos::Support::PacketComm packet;
    uint32_t send_counter = 0;
    elapsedMillis telem_timer;
    elapsedMillis transmit_timer;
    bool initialized = false;
    Astrodev::frame tmessage;
}

void setup()
{
    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // Setup serial stuff
    Serial.begin(115200);
    shared.astrodev_tx.setSerial(&HWSERIAL);
    HWSERIAL.begin(ASTRODEV_BAUD);
    HWSERIAL.flush();
    HWSERIAL.clear();

    // Each thread tick length
    threads.setSliceMicros(10);

    threads.addThread(Cosmos::Module::Radio_interface::tx_recv_loop, 0, RX_STACK_SIZE);

    Serial.println("Setup complete, starting test_ground");
}

void loop()
{
    // Process command-type packets for this program
    handle_main_queue_packets();

    // Wait until main switch board thinks this radio is successfully initialized
    if (initialized)
    {
        // Send transmit packets every second
        if (transmit_timer > 1000)
        {
            transmit_timer = 0;
            send_test_transmit_packet();
        }
    }

    
    
    threads.delay(10);
}

// Any packets inside the main queue are command-type packets
// intended for this program to process in some way.
void handle_main_queue_packets()
{
    // Send all received packets to iobc
    iretn = shared.pop_queue(shared.main_queue, shared.main_lock, packet);
    if (iretn >= 0)
    {
#ifdef DEBUG_PRINT
        char msg[4];
        Serial.print("main: ");
        for (uint16_t i=0; i<packet.data.size(); i++)
        {
            sprintf(msg, "0x%02X", packet.data[i]);
            Serial.print(msg);
            Serial.print(" ");
        }
        Serial.println();
#endif
        using namespace Cosmos::Support;
        switch(packet.header.type)
        {
        case PacketComm::TypeId::CommandPing:
            {
                Serial.println("Pong!");
            }
            break;
        case PacketComm::TypeId::CommandRadioCommunicate:
            {
                
            }
            break;
        case PacketComm::TypeId::DataRadioResponse:
            {
                Serial.print("Radio response, cmd: ");
                Serial.println(packet.data[0]);
                fake_radio_response_creator((Astrodev::Command)packet.data[0]);
            }
            break;
        default:
            {
                Serial.println("Packet type not handled, exiting...");
                exit(-1);
            }
            break;
        }
    }
}

// Handle stuff that comes in over the Astrodev library,
// I say "fake" because there's no actual radio being used here
void fake_radio_response_creator(Astrodev::Command cmd)
{
    Astrodev::frame message;
    switch(cmd)
    {
    case Astrodev::Command::NOOP:
        {
            message.header.command = Astrodev::Command::NOOP;
            message.header.sizehi = 0x0a;
            message.header.sizelo = 0x0a;
            fake_transmit(message, 0);
        }
        break;
    case Astrodev::Command::RESET:
        {
            // Stop sending packets on reset
            initialized = false;

            message.header.command = Astrodev::Command::RESET;
            message.header.sizehi = 0x0a;
            message.header.sizelo = 0x0a;
            fake_transmit(message, 0);
        }
        break;
    case Astrodev::Command::GETTCVCONFIG:
        {
            message.header.command = Astrodev::Command::GETTCVCONFIG;
            message.header.sizehi = 0;
            message.header.sizelo = sizeof(shared.astrodev_tx.tcv_configuration);
            shared.astrodev_tx.tcv_configuration.interface_baud_rate = 0;
            shared.astrodev_tx.tcv_configuration.power_amp_level = 220;
            shared.astrodev_tx.tcv_configuration.rx_baud_rate = 1;
            shared.astrodev_tx.tcv_configuration.tx_baud_rate = 1;
            shared.astrodev_tx.tcv_configuration.ax25_preamble_length = 20;
            shared.astrodev_tx.tcv_configuration.ax25_postamble_length = 20;
            shared.astrodev_tx.tcv_configuration.rx_modulation = Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
            shared.astrodev_tx.tcv_configuration.tx_modulation = Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
            shared.astrodev_tx.tcv_configuration.tx_frequency = 400800;
            shared.astrodev_tx.tcv_configuration.rx_frequency = 449900;
            memcpy(shared.astrodev_tx.tcv_configuration.ax25_source, "SOURCE", 6);
            memcpy(shared.astrodev_tx.tcv_configuration.ax25_destination, "DESTIN", 6);
            memcpy(&message.payload[0], &shared.astrodev_tx.tcv_configuration, sizeof(shared.astrodev_tx.tcv_configuration));
            fake_transmit(message, sizeof(shared.astrodev_tx.tcv_configuration));
            
            // Li3 switcher board radio initialization completes when this GETTCVCONFIG returns successfully
            threads.delay(1000);
            initialized = true;
        }
        break;
    case Astrodev::Command::SETTCVCONFIG:
        {
            message.header.command = Astrodev::Command::SETTCVCONFIG;
            message.header.sizehi = 0x0a;
            message.header.sizelo = 0x0a;
            fake_transmit(message, 0);
        }
        break;
    default:
        Serial.println("Command type not handled or implemented. Exiting...");
        break;
    }
}

// Must set sizelo and sizehi manually to account for faking an ACK/NOACK response,
// and the actual size of the payload must be provided here as an argument.
// The command type is the direction of the message, RESPONSE is for faking a radio response,
// COMMAND is for making an actual radio command.
int32_t fake_transmit(Astrodev::frame &message, uint8_t size)
{
    // Wait a little bit
    threads.delay(10);
    message.header.sync0 = Astrodev::SYNC0;
    message.header.sync1 = Astrodev::SYNC1;
    // These frames are mocks of incoming stuff from the radio, so type if RESPONSE
    message.header.type = Astrodev::RESPONSE;
    message.header.cs = shared.astrodev_tx.CalcCS(&message.preamble[2], 4);
    HWSERIAL.write(&message.preamble[0], 8);

    // Skip payload and payload crc if header-only
    if (!size)
    {
        return size;
    }

    union
    {
        uint16_t cs;
        uint8_t csb[2];
    };
    cs = shared.astrodev_tx.CalcCS(&message.preamble[2], 6+message.header.sizelo);
    message.payload[message.header.sizelo] = csb[0];
    message.payload[message.header.sizelo+1] = csb[1];

    for (uint16_t i=0; i<message.header.sizelo+2; i++)
    {
        HWSERIAL.write(message.payload[i]);
    }
    // 10 = 8 bytes header + 2 byte crc
    return message.header.sizelo + 10;
}

// Fake a sending of a PacketComm packet
int32_t fake_transmit(Cosmos::Support::PacketComm &packet)
{
    // Apply packetcomm protocol wrapping
    int32_t iretn = packet.Wrap();
    if (iretn < 0)
    {
        return iretn;
    }

    // Other side will receive it, so this is RECEIVE type
    tmessage.header.command = Astrodev::Command::RECEIVE;
    tmessage.header.sizelo = packet.wrapped.size();
    tmessage.header.sizehi = 0;
    if (tmessage.header.sizelo > Astrodev::MTU)
    {
        return GENERAL_ERROR_BAD_SIZE;
    }

    memcpy(&tmessage.payload[0], packet.wrapped.data(), packet.wrapped.size());
    
    return fake_transmit(tmessage, packet.wrapped.size());
}

// Send a test packet from the "ground" to the iobc
void send_test_transmit_packet()
{
    packet.header.type = PacketComm::TypeId::CommandPing;
    packet.header.orig = GROUND_NODE_ID;
    packet.header.dest = IOBC_NODE_ID;
    packet.header.radio = 0;
    const size_t REPEAT = 124;
    // ascend from 0 to REPEAT-1, then last 4 bytes is packet number
    packet.data.resize(REPEAT + sizeof(send_counter));
    for (size_t i = 0; i < REPEAT; ++i)
    {
        packet.data[i] = i & 0xFF;
    }
    memcpy(packet.data.data()+REPEAT, &send_counter, sizeof(send_counter));
    iretn = packet.Wrap();
    if (iretn)
    {
        // turn the LED on (HIGH is the voltage level)
        digitalWrite(LED_BUILTIN, HIGH);
        threads.delay(10);
        fake_transmit(packet);
#ifdef DEBUG_PRINT
        Serial.print("wrapped.size:");
        Serial.println(packet.wrapped.size());
        char msg[4];
        Serial.print("wrapped: ");
        for (uint16_t i=0; i<packet.wrapped.size(); i++)
        {
            sprintf(msg, "0x%02X", packet.wrapped[i]);
            Serial.print(msg);
            Serial.print(" ");
        }
        Serial.println();
#endif
        // turn the LED off by making the voltage LOW
        digitalWrite(LED_BUILTIN, LOW);
        threads.delay(10);
    }
    packet.data.clear();
    ++send_counter;
    // wait for a bit
    threads.delay(10);
}
