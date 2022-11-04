#include "shared_resources.h"

int32_t shared_resources::init_radio(HardwareSerial* new_serial, uint32_t speed)
{
    int32_t iretn = astrodev.Init(new_serial, 9600);
    if (iretn < 0)
    {
        Serial.println("Error initializing Astrodev radio. Exiting...");
        exit(-1);
    }
    astrodev.tcv_configuration.interface_baud_rate = 0;
    astrodev.tcv_configuration.power_amp_level = 220;
    astrodev.tcv_configuration.rx_baud_rate = 1;
    astrodev.tcv_configuration.tx_baud_rate = 1;
    astrodev.tcv_configuration.ax25_preamble_length = 20;
    astrodev.tcv_configuration.ax25_postamble_length = 20;
    // astrodev.tcv_configuration.config1;
    // astrodev.tcv_configuration.config2;
    astrodev.tcv_configuration.rx_modulation = (uint8_t)Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
    astrodev.tcv_configuration.tx_modulation = (uint8_t)Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
    astrodev.tcv_configuration.tx_freq_high = 450000 / 65536;
    astrodev.tcv_configuration.tx_freq_low = 450000 % 65536;
    astrodev.tcv_configuration.rx_freq_high = 450000 / 65536;
    astrodev.tcv_configuration.rx_freq_low = 450000 % 65536;
    memcpy(astrodev.tcv_configuration.ax25_source, "SOURCE", 6);
    memcpy(astrodev.tcv_configuration.ax25_destination, "DESTIN", 6);

    int8_t retries = 5;

    while ((iretn = astrodev.SetTCVConfig()) < 0)
    {
        Serial.println("Resetting");
        astrodev.Reset();
        Serial.println("Failed to settcvconfig astrodev");
        if (--retries < 0)
        {
            return iretn;
        }
        threads.delay(5000);
    }
    Serial.println("SetTCVConfig successful");

    retries = 5;
    while ((iretn = astrodev.GetTCVConfig()) < 0)
    {
        Serial.println("Failed to gettcvconfig astrodev");
        if (--retries < 0)
        {
            return iretn;
        }
        threads.delay(5000);
    }
    Serial.println("GetTCVConfig successful");
    return 0;
}

// Return -1 if buffer is empty, size of recv_buffer on success
int32_t shared_resources::pop_recv(Cosmos::Support::PacketComm &packet)
{
  Threads::Scope lock(recv_lock);
  //std::vector<uint8_t> packet (std::make_move_iterator(recv_buffer.front().begin()), std::make_move_iterator(recv_buffer.front().end()));
  if (!recv_buffer.size())
  {
      return -1;
  }
  packet = recv_buffer.front();
  recv_buffer.pop_front();
  return recv_buffer.size();
}

void shared_resources::push_recv(const Cosmos::Support::PacketComm &packet)
{
  Threads::Scope lock(recv_lock);
  recv_buffer.push_back(packet);
}

// Return -1 if buffer is empty, size of send_buffer on success
int32_t shared_resources::pop_send(Cosmos::Support::PacketComm &packet)
{
  Threads::Scope lock(send_lock);
  if (!send_buffer.size())
  {
      return -1;
  }
  //std::vector<uint8_t> packet (std::make_move_iterator(recv_buffer.front().begin()), std::make_move_iterator(recv_buffer.front().end()));
  packet = send_buffer.front();
  send_buffer.pop_front();
  return send_buffer.size();
}

void shared_resources::push_send(const Cosmos::Support::PacketComm &packet)
{
  Threads::Scope lock(send_lock);
  send_buffer.push_back(packet);
}