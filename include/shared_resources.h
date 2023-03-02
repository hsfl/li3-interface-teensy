#ifndef _SHARED_RESOURCES_H
#define _SHARED_RESOURCES_H

#include <Arduino.h>
#include <map>
#include <TeensyThreads.h>
#include <SLIPEncodedSerial.h>
#include <deque>

#include "common_def.h"
#include "astrodev.h"
#include "support/packetcomm.h"

class shared_resources
{
private:
    
public:

    shared_resources(HardwareSerial& hwserial);

    // These are the UART serial pins that communicate with the iobc
    HardwareSerial *IobcSerial;
    SLIPEncodedSerial SLIPIobcSerial;


    // Astrodev radio, has its own serial pin for read/write
    Cosmos::Devices::Radios::Astrodev astrodev_rx;
    Cosmos::Devices::Radios::Astrodev astrodev_tx;
    //! Initializes RX and TX Astrodev radios
    int32_t init_radios(HardwareSerial* hw_serial_rx, HardwareSerial* hw_serial_tx, uint32_t baud_rate);
    int32_t connect_radio(Cosmos::Devices::Radios::Astrodev& astrodev, uint32_t tx_freq, uint32_t rx_freq);

    // See if these can't be replaced by 2D arrays
    std::deque<Cosmos::Support::PacketComm> send_queue;
    std::deque<Cosmos::Support::PacketComm> main_queue;

    // Mutexes for accessing buffers
    Threads::Mutex send_lock;
    Threads::Mutex main_lock;

    // Buffer accessors
    int32_t pop_queue(std::deque<Cosmos::Support::PacketComm>& queue, Threads::Mutex& mutex, Cosmos::Support::PacketComm &packet);
    void push_queue(std::deque<Cosmos::Support::PacketComm>& queue, Threads::Mutex& mutex, const Cosmos::Support::PacketComm &packet);

    // Set burnwire pin to LOW when burnwire_timer exceeds this value
    uint32_t burnwire_on_time = 0;
    // Control timer for how long burnwire is on for
    elapsedMillis burnwire_timer;
    // Keep track of state of burnwire
    uint8_t burnwire_state = HIGH;

    void set_radios_initialized_state(bool state);
    bool get_radios_initialized_state();
    void set_radios_threads_started(bool state);
    bool get_radios_threads_started();
private:
    int32_t init_radio(Cosmos::Devices::Radios::Astrodev &astrodev, HardwareSerial* hw_serial, uint32_t baud_rate, uint32_t tx_freq, uint32_t rx_freq);
    // Keep track of whether radio initialization state
    bool radios_initialized = false;
    // Only start radio threads once
    bool radio_threads_started = false;
};

#endif
