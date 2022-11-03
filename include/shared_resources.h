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

    shared_resources(HardwareSerial& hwserial) : HWSerial(&hwserial), SLIPHWSerial(hwserial)  {}

    // Define hardware pin and wrap with SLIP helper class
    // These are separate from the radio read/write pins,
    // for use within the queue loops
    HardwareSerial *HWSerial;
    SLIPEncodedSerial SLIPHWSerial;


    // Astrodev radio, has its own serial pin for read/write
    Cosmos::Devices::Radios::Astrodev astrodev;
    int32_t init_radio(HardwareSerial* new_serial, uint32_t speed);

    // See if these can't be replaced by 2D arrays
    std::deque<Cosmos::Support::PacketComm> recv_buffer;
    std::deque<Cosmos::Support::PacketComm> send_buffer;

    // Mutexes for accessing buffers
    Threads::Mutex recv_lock;
    Threads::Mutex send_lock;

    // Buffer accessors
    int32_t pop_recv(Cosmos::Support::PacketComm &packet);
    void push_recv(const Cosmos::Support::PacketComm &packet);
    int32_t pop_send(Cosmos::Support::PacketComm &packet);
    void push_send(const Cosmos::Support::PacketComm &packet);

};

#endif
