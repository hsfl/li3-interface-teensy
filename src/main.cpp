/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include <Arduino.h>
#include <TeensyThreads.h>
#include "support/cosmos-defs.h"

#define RXS_PACKET_SIZE 250
#define TXS_PACKET_SIZE 250

// See if these can't be replaced by 2D arrays
std::vector<std::vector<uint8_t>> send_buffer;
std::vector<std::vector<uint8_t>> recv_buffer;

int rnum = 0;
int snum = 0;

// Recv thread loop
void recv_loop();

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // Start recv loop
  threads.addThread(recv_loop);
  threads.setSliceMicros(10);
}

// TXS Loop
// Empties array 
void loop()
{
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);
  // wait for a second
  threads.delay(1000);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
  // wait for a second
  threads.delay(1000);
  Serial.printf("send %d\n", snum++);
}

// RXS Loop
void recv_loop()
{
  // Wait for main loop to start
  delay(1000);

  while(true)
  {
    Serial.printf("rec %d\n", rnum++);
    threads.delay(1000);
    //threads.yield();
  }
}

