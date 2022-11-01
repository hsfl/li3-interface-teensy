/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include <Arduino.h>
#include <map>
#include <TeensyThreads.h>
#include <SLIPEncodedSerial.h>

#include <iterator>
#include "support/cosmos-defs.h"
#include "support/packetcomm.h"
#include "astrodev.h"
// #include "math/bytelib.h"
// #include "math/crclib.h"

#define RXS_PACKET_SIZE 250
#define TXS_PACKET_SIZE 250

#define HWSERIAL Serial5
SLIPEncodedSerial SLIPHWSerial(HWSERIAL);

// See if these can't be replaced by 2D arrays
std::deque<std::vector<uint8_t>> recv_buffer;
std::deque<std::vector<uint8_t>> send_buffer;

// Mutexes for accessing buffers
Threads::Mutex recv_lock;
Threads::Mutex send_lock;

// Buffer accessors
std::vector<uint8_t> pop_recv();
void push_recv(std::vector<uint8_t> packet);
std::vector<uint8_t> pop_send();
void push_send(std::vector<uint8_t> packet);

Cosmos::Devices::Radios::Astrodev astrodev;
// Recv thread loop
void recv_loop();

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // Each thread tick length
  threads.setSliceMicros(10);
  // Wait for serial monitor
  Serial.println("Delay 5 sec");
  delay(5000);

  // Setup serial stuff
  Serial.begin(115200);
  int32_t iretn = astrodev.Init(&Serial8, 9600);
  if (iretn < 0)
  {
    Serial.println("Error initializing Astrodev radio. Exiting...");
    exit(-1);
  }

  astrodev.tcv_config.interface_baud_rate = 0;
  astrodev.tcv_config.power_amp_level = 220;
  astrodev.tcv_config.rx_baud_rate = 1;
  astrodev.tcv_config.tx_baud_rate = 1;
  astrodev.tcv_config.ax25_preamble_length = 20;
  astrodev.tcv_config.ax25_postamble_length = 20;
  // astrodev.tcv_config.config1;
  // astrodev.tcv_config.config2;
  astrodev.tcv_config.rx_modulation = (uint8_t)Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
  astrodev.tcv_config.tx_modulation = (uint8_t)Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
  astrodev.tcv_config.tx_freq_high = 450000 / 65536;
  astrodev.tcv_config.tx_freq_low = 450000 % 65536;
  astrodev.tcv_config.rx_freq_high = 450000 / 65536;
  astrodev.tcv_config.rx_freq_low = 450000 % 65536;
  memcpy(astrodev.tcv_config.ax25_source, "SOURCE", 6);
  memcpy(astrodev.tcv_config.ax25_destination, "DESTIN", 6);

  while ((iretn = astrodev.SetTCVConfig()) < 0)
  {
    Serial.println("Resetting");
    astrodev.Reset();
    Serial.println("Failed to settcvconfig astrodev");
    threads.delay(5000);
  }
  Serial.println("SetTCVConfig successful");
  while ((iretn = astrodev.GetTCVConfig()) < 0)
  {
    Serial.println("Failed to gettcvconfig astrodev");
    threads.delay(5000);
  }
  Serial.println("GetTCVConfig successful");
  Serial.println("Exiting...");
  exit(-1);

  /*
  SLIPHWSerial.begin(115200);
  Serial.clear();
  HWSERIAL.clear();

  // Start recv loop
  //threads.addThread(recv_loop);
  */
}

PacketComm packet;
int32_t iretnSend = 0;

int32_t sentNum = 0;
// TXS Loop
// Empties array
void loop()
{
  /*
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);
  //Serial.println("sending!");
  packet.header.type = PacketComm::TypeId::CommandClearQueue;
  packet.header.orig = 1;
  packet.header.dest = 2;
  packet.header.radio = 3;
  packet.header.data_size = 12;
  char str[12] = "hello world";
  packet.data.insert(packet.data.end(), str, str + 12);
  iretnSend = packet.SLIPPacketize();
  if (iretnSend)
  {
    HWSERIAL.write(packet.packetized.data(), packet.packetized.size());
  }
  packet.data.clear();
  // SLIPHWSerial.beginPacket();
  // SLIPHWSerial.print(str);
  // SLIPHWSerial.endPacket();
  // wait for a bit
  threads.delay(10);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
  //Serial.println("send sleeping...");
  */

  // Ping code
  // Serial.print(sentNum++);
  // Serial.print(" | Pinging...");
  // int32_t iretn = astrodev.Ping();
  // Serial.print(" iretn: ");
  // Serial.println(iretn);

  // Testing message transmissions

  Serial.print(sentNum++);
  Serial.println(" | Transmit message...");
  Cosmos::Devices::Radios::Astrodev::frame message;
  message.header.command = (uint8_t)Cosmos::Devices::Radios::Astrodev::Command::TRANSMIT;
  message.header.sizehi = 0;
  message.header.sizelo = 25;
  for (size_t i = 0; i < message.header.sizelo; ++i)
  {
    message.payload[i] = i;
  }
  int32_t iretn = astrodev.Transmit(message);
  Serial.print(" iretn: ");
  Serial.println(iretn);

  // wait for 3 seconds
  threads.delay(10);
  if (sentNum > 6000)
  {
    exit(-1);
  }
  //threads.idle();
}

PacketComm packetRecv;
char buf[256];
// RXS Loop
void recv_loop()
{
  // Wait for main loop to start
  int32_t iretnRecv = 0;
  delay(1000);
  while (true)
  {
    uint16_t size = 0;
    uint8_t buflen = HWSERIAL.available();
    packetRecv.wrapped.resize(256);
    char msg[16];
    sprintf(msg, "available: %03d", buflen);
    Serial.println(msg);
    while(!SLIPHWSerial.endofPacket())
    {
      // len is either 0 (done) or 1 (not end)
      uint8_t len = SLIPHWSerial.available();
      // If there are bytes in receive buffer to be read
      if (len > 0)
      {
        // Read a slip packet
        while(len--)
        {
          // Note: goes into wrapped because SLIPHWSerial.read()
          // already decodes SLIP encoding
          packetRecv.wrapped[size] = SLIPHWSerial.read();
          //buf[size] = SLIPHWSerial.read();
          size++;
        }
      }
    }
    packetRecv.wrapped.resize(size);
    // iretnRecv = packetRecv.SLIPUnPacketize();
    iretnRecv = packetRecv.Unwrap();
    Serial.print("iretnRecv: ");
    Serial.print(iretnRecv);
    Serial.print(" size ");
    Serial.print(size);
    Serial.print(" wrapped.size ");
    Serial.print(packetRecv.wrapped.size());
    Serial.print(" data.size ");
    Serial.println(packetRecv.data.size());
    for (size_t i=0; i < packetRecv.wrapped.size(); ++i) {
      Serial.print(packetRecv.wrapped[i]);
      Serial.print(" ");
    }
    Serial.println();
    for (size_t i=0; i < packetRecv.data.size(); ++i) {
      Serial.print(char(packetRecv.data[i]));
    }
    Serial.println();
    Serial.println("now sleeping...");
    threads.delay(3000);
    threads.yield();
  }
}



std::vector<uint8_t> pop_recv()
{
  Threads::Scope lock(recv_lock);
  //std::vector<uint8_t> packet (std::make_move_iterator(recv_buffer.front().begin()), std::make_move_iterator(recv_buffer.front().end()));
  std::vector<uint8_t> packet = recv_buffer.front();
  recv_buffer.pop_front();
  return packet;
}

void push_recv(std::vector<uint8_t> packet)
{
  Threads::Scope lock(recv_lock);
  recv_buffer.push_back(packet);
}

std::vector<uint8_t> pop_send()
{
  Threads::Scope lock(send_lock);
  //std::vector<uint8_t> packet (std::make_move_iterator(recv_buffer.front().begin()), std::make_move_iterator(recv_buffer.front().end()));
  std::vector<uint8_t> packet = send_buffer.front();
  send_buffer.pop_front();
  return packet;
}

void push_send(std::vector<uint8_t> packet)
{
  Threads::Scope lock(send_lock);
  send_buffer.push_back(packet);
}

