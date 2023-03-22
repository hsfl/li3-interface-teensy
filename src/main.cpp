#include "shared_resources.h"
#include "helpers/RadioCommands.h"
#include "module/radio_recv.h"
#include "module/radio_send.h"
#include "module/tx_radio_loop.h"
#include "module/iobc_recv.h"
#include "helpers/TestBlinker.h"

// For setting Teensy Clock Frequency (only for Teensy 4.0 and 4.1)
#if defined(__IMXRT1062__)
extern "C" uint32_t set_arm_clock(uint32_t frequency);
#endif

// Pins for AD590MF temp sensor
#define MUXA 5
#define MUXB 6
#define MUXC 9
#define AD590MF_TEMP_PIN 19

// Function forward declarations
void sendpacket(Cosmos::Support::PacketComm &packet);
void initialize_radios();
void handle_main_queue_packets();
void get_temp_sensor_measurements();
void control_burnwire();

elapsedMillis temp_timer;

//! Global shared resources defined here
shared_resources shared(Serial1);

namespace
{
    int32_t iretn = 0;
    // Reusable packet objects
    Cosmos::Support::PacketComm packet;
}

void setup()
{
    // Setup serial stuff
    Serial.begin(115200);

    #if defined(__IMXRT1062__)
        set_arm_clock(150000000); // Allowed Frequencies (MHz): 24, 150, 396, 450, 528, 600
    #endif

    // initialize LED digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);

    // Each thread tick length
    threads.setSliceMicros(10);

    // TODO: determine more appropriate stack size
    threads.addThread(Cosmos::Module::Radio_interface::iobc_recv_loop, 0, RX_STACK_SIZE);

    Serial.println("Setup complete");

}

//! Main loop
void loop()
{
    // Initialize radios if it they are not yet
    if (!shared.get_rx_radio_initialized_state() || !shared.get_tx_radio_initialized_state())
    {
        initialize_radios();
        threads.delay(10);
    }

    // Check burnwire timer
    control_burnwire();

    // Get temperatures
    get_temp_sensor_measurements();

    // Process command-type packets for this program
    handle_main_queue_packets();

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
        case PacketComm::TypeId::CommandRadioAstrodevCommunicate:
            {
                // These packets are intended for this program, handle here
                if (packet.header.nodedest == LI3TEENSY_ID || packet.header.nodeorig == GROUND_NODE_ID)
                {
                    Lithium3::RadioCommand(packet);
                }
                // These are our periodic telem grabbing responses, send to iobc
                else if (packet.header.nodeorig == IOBC_NODE_ID)
                {
                    Serial.print("Got radio communicate response unit:");
                    Serial.print(packet.data[0]);
                    Serial.print(" type:");
                    Serial.print(packet.data[2]);
                    Serial.println(", forwarding to iobc");
                    iretn = packet.Wrap();
                    if (iretn < 0)
                    {
                        Serial.println("Error in Wrap()");
                    }
                    shared.SLIPIobcSerial.beginPacket();
                    shared.SLIPIobcSerial.write(packet.wrapped.data(), packet.wrapped.size());
                    shared.SLIPIobcSerial.endPacket();
                }
            }
            break;
        case PacketComm::TypeId::DataRadioResponse:
            {
                Serial.print("Radio response, cmd: ");
                Serial.println(packet.data[0]);
            }
            break;
        default:
            {
                Serial.print("Packet type ");
                Serial.print((uint16_t)packet.header.type);
                Serial.println(" not handled, forwarding to iobc");
                iretn = packet.Wrap();
                if (iretn < 0)
                {
                    Serial.println("Error in Wrap()");
                }
                shared.SLIPIobcSerial.beginPacket();
                shared.SLIPIobcSerial.write(packet.wrapped.data(), packet.wrapped.size());
                shared.SLIPIobcSerial.endPacket();
            }
            break;
        }
    }
}

void initialize_radios()
{
    // Initialize the astrodev radios

    // Start send/receive loops
    // Do it only once
    if (!shared.get_rx_radio_thread_started())
    {
        if (shared.init_rx_radio(&Serial5, ASTRODEV_BAUD) >= 0)
        {
            threads.addThread(Cosmos::Module::Radio_interface::rx_recv_loop, 0, RX_STACK_SIZE);
            shared.set_rx_radio_thread_started(true);
        }
    }
    if (!shared.get_tx_radio_thread_started())
    {
        if (shared.init_tx_radio(&Serial2, ASTRODEV_BAUD) >= 0)
        {
            threads.addThread(Cosmos::Module::Radio_interface::tx_recv_loop, 0, RX_STACK_SIZE);
            threads.addThread(Cosmos::Module::Radio_interface::send_loop, 0, TX_STACK_SIZE);
            // threads.addThread(Cosmos::Module::Radio_interface::tx_radio_loop, 0, TX_STACK_SIZE);
            shared.set_tx_radio_thread_started(true);
        }
    }
    // Serial.println("Radio init successful");
}

void get_temp_sensor_measurements()
{
    // Get temperature every 10 seconds
    if (temp_timer < 10000)
    {
        return;
    }
    temp_timer = 0;

    // Stick temp measurements under special kind of CommandRadioAstrodevCommunicate packet
    // Byte 0 = Unit (0)
    // Byte 1 = doesn't matter
    // Byte 2 = 253
    // Byte 3 = Number of response bytes (8)
    // Byte 4-7  = TSEN 0 temp (iX5 heat sink +y)
    // Byte 8-11 = TSEN 1 temp (iX5 heat sink -y)
    packet.header.type = PacketComm::TypeId::CommandRadioAstrodevCommunicate;
    packet.header.nodeorig = IOBC_NODE_ID;
    packet.header.nodedest = IOBC_NODE_ID;
    packet.data.resize(CMD_HEADER_SIZE+sizeof(float)*NUM_TSENS);
    packet.data[0] = 0;
    packet.data[1] = 0;
    packet.data[2] = TLM_TSENS;
    packet.data[3] = 8;

    // AD590MF temp sensor
    pinMode(AD590MF_TEMP_PIN, INPUT);
    analogReadResolution(10);
    // Get measurements for each of the 8 temp sensors
    // The connected sensor is selected by turning 3 bits/pins hi or lo
    // Only sensors 0 and 1 are connected
    for (uint8_t i=0; i < NUM_TSENS; ++i)
    {
        // Measurement for temp sensor n: 0000 0cba
        uint8_t a_out = i & 0x1;
        uint8_t b_out = (i & 0x2) >> 1;
        uint8_t c_out = (i & 0x4) >> 2;
        pinMode(MUXA, OUTPUT); digitalWrite(MUXA, a_out);
        pinMode(MUXB, OUTPUT); digitalWrite(MUXB, b_out);
        pinMode(MUXC, OUTPUT); digitalWrite(MUXC, c_out);
        Serial.print("TEMP SENSOR(");
        Serial.print(unsigned(i));
        Serial.print("): ");
        // 10 bits of read resolution (0~1023)
        // 1 mV/K
        // Reference voltage 3.3V
        float volt = (analogRead(AD590MF_TEMP_PIN)/1024.) * 3.3;
        // V = IR    =>    I = V/R
        // R = 7.77 kOhms
        // Temp (K) = I (uA) = V/R * 1000000
        float kelvin = volt / .00777;
        Serial.println(kelvin-273.15);
        memcpy(&packet.data[CMD_HEADER_SIZE+i*sizeof(kelvin)], &kelvin, sizeof(kelvin));
    }
    pinMode(AD590MF_TEMP_PIN, INPUT_DISABLE);

    shared.push_queue(shared.main_queue, shared.main_lock, packet);
    
    delay(1000);
}

void control_burnwire()
{
    // Set burnwire pin to LOW after time exceeds on time
    // If burnwire_on_time is 0, then the burnwire is kept HIGH until manually commanded to turn LOW
    if (shared.burnwire_state == HIGH && shared.burnwire_on_time && shared.burnwire_timer > shared.burnwire_on_time)
    {
        shared.burnwire_state = LOW;
        Serial.print("Setting burn wire ");
        Serial.println("LOW");
        pinMode(12, OUTPUT);
        digitalWrite(12, LOW);
    }
}