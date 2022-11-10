#include "shared_resources.h"

static uint8_t READ_BUFFER[READ_BUFFER_SIZE];

shared_resources::shared_resources(HardwareSerial& hwserial) : IobcSerial(&hwserial), SLIPIobcSerial(hwserial)
{
    IobcSerial->addMemoryForRead(&READ_BUFFER, READ_BUFFER_SIZE);
    SLIPIobcSerial.begin(115200);
    IobcSerial->clear();
    SLIPIobcSerial.flush();
}

int32_t shared_resources::init_radios(HardwareSerial* hw_serial_rxs, HardwareSerial* hw_serial_txs, uint32_t baud_rate)
{
    int32_t iretn = 0;
    // Serial.println("Initializing RXS");
    // iretn = init_radio(astrodev_rxs, hw_serial_rxs, baud_rate);
    // if (iretn < 0)
    // {
    //     Serial.println("RXS Initialization failed");
    //     return iretn;
    // }
    // Serial.println("RXS Initialization success");
    Serial.println("Initializing TXS");
    iretn = init_radio(astrodev_txs, hw_serial_txs, baud_rate);
    if (iretn < 0)
    {
        Serial.println("TXS Initialization failed");
        return iretn;
    }
    Serial.println("TXS Initialization success");
    Serial.println("RXS and TXS succesfully initialized!");

    return 0;
}

int32_t shared_resources::init_radio(Cosmos::Devices::Radios::Astrodev &astrodev, HardwareSerial* hw_serial, uint32_t baud_rate)
{
    int32_t iretn = astrodev.Init(hw_serial, baud_rate);
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

// Return -1 if buffer is empty, size of queue on success
int32_t shared_resources::pop_queue(std::deque<Cosmos::Support::PacketComm>& queue, Threads::Mutex& mutex, Cosmos::Support::PacketComm &packet)
{
  Threads::Scope lock(mutex);
  if (!queue.size())
  {
      return -1;
  }
  //std::vector<uint8_t> packet (std::make_move_iterator(recv_queue.front().begin()), std::make_move_iterator(recv_queue.front().end()));
  packet = queue.front();
  queue.pop_front();
  return queue.size();
}

// Push a packet onto a queue
void shared_resources::push_queue(std::deque<Cosmos::Support::PacketComm>& queue, Threads::Mutex& mutex, const Cosmos::Support::PacketComm &packet)
{
  Threads::Scope lock(mutex);
  queue.push_back(packet);
}
