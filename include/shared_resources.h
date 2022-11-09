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
    Cosmos::Devices::Radios::Astrodev astrodev;
    int32_t init_radio(HardwareSerial* new_serial, uint32_t speed);

    // See if these can't be replaced by 2D arrays
    // std::deque<Cosmos::Support::PacketComm> recv_queue;
    std::deque<Cosmos::Support::PacketComm> send_queue;
    std::deque<Cosmos::Support::PacketComm> main_queue;

    // Mutexes for accessing buffers
    // Threads::Mutex recv_lock;
    Threads::Mutex send_lock;
    Threads::Mutex main_lock;

    // Buffer accessors
    int32_t pop_queue(std::deque<Cosmos::Support::PacketComm>& queue, Threads::Mutex& mutex, Cosmos::Support::PacketComm &packet);
    void push_queue(std::deque<Cosmos::Support::PacketComm>& queue, Threads::Mutex& mutex, const Cosmos::Support::PacketComm &packet);

};

#endif
