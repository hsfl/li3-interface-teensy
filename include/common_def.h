#ifndef _COMMON_DEF_H
#define _COMMON_DEF_H

#define ASTRODEV_BAUD 9600

#define RX_PACKET_SIZE 250
#define TX_PACKET_SIZE 250

#define RX_STACK_SIZE 9000
#define TX_STACK_SIZE 6000
#define MAX_QUEUE_SIZE 50

#define READ_BUFFER_SIZE 512

// Node ids from nodeids.ini, hardcoded here
#define GROUND_NODE_ID 1
#define UNIBAP_NODE_ID 10
#define IOBC_NODE_ID 12
#define LI3TEENSY_ID 112 // For internal use to be intended for internal use

// TODO: Change to actual radio num later
#define LI3RX 0
#define LI3TX 1

// Number of times to attempt reconnect of radios during initialization
#define RADIO_INIT_CONNECT_ATTEMPTS 10

// For rebooting the teensy on software init failure
#define RESTART_ADDR       0xE000ED0C
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))

// Number of thermal sensors attached to teensy
#define NUM_TSENS 2

// Special internal command ids
#define CMD_HEADER_SIZE 4
#define TLM_TSENS 253
#define CMD_BURNWIRE 254
#define CMD_REBOOT 255

#endif
