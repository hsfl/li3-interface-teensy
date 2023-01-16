#ifndef _COMMON_DEF_H
#define _COMMON_DEF_H

#define ASTRODEV_BAUD 9600

#define RX_PACKET_SIZE 250
#define TX_PACKET_SIZE 250

#define RX_STACK_SIZE 6000
#define TX_STACK_SIZE 6000
#define MAX_QUEUE_SIZE 50

#define READ_BUFFER_SIZE 512

// Node ids from nodeids.ini, hardcoded here
#define GROUND_NODE_ID 1
#define IOBC_NODE_ID 12

// TODO: Change to actual radio num later
#define LI3RX 0
#define LI3TX 1

#endif
