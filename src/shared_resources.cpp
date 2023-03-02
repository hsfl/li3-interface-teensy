#include "shared_resources.h"
#include "helpers/TestBlinker.h"

static uint8_t READ_BUFFER[READ_BUFFER_SIZE];

shared_resources::shared_resources(HardwareSerial& hwserial) : IobcSerial(&hwserial), SLIPIobcSerial(hwserial)
{
    IobcSerial->addMemoryForRead(&READ_BUFFER, READ_BUFFER_SIZE);
    SLIPIobcSerial.begin(9600);
    SLIPIobcSerial.println("Serial Begin");
    IobcSerial->clear();
    SLIPIobcSerial.flush();
}

int32_t shared_resources::init_radios(HardwareSerial* hw_serial_rx, HardwareSerial* hw_serial_tx, uint32_t baud_rate)
{
    using namespace Lithium3;
    int32_t iretn = 0;
    Serial.println("Initializing RX");
    BlinkPattern(ProgramState::RADIO_RX_ATTEMPT_INIT);
    iretn = init_radio(astrodev_rx, hw_serial_rx, baud_rate, 449900, 449900);
    if (iretn < 0)
    {
        Serial.println("RX Initialization failed");
        BlinkPattern(ProgramState::RADIO_RX_INIT_FAIL);
        BlinkPattern(ProgramState::INIT_FAIL);
        return iretn;
    }
    Serial.println("RX Initialization success");
    BlinkPattern(ProgramState::RADIO_RX_INIT_SUCCESS);
    Serial.println("Initializing TX");
    BlinkPattern(ProgramState::RADIO_TX_ATTEMPT_INIT);
    iretn = init_radio(astrodev_tx, hw_serial_tx, baud_rate, 400800, 400800);
    if (iretn < 0)
    {
        Serial.println("TX Initialization failed");
        BlinkPattern(ProgramState::RADIO_TX_INIT_FAIL);
        BlinkPattern(ProgramState::INIT_FAIL);
        return iretn;
    }
    Serial.println("TX Initialization success");
    BlinkPattern(ProgramState::RADIO_TX_INIT_SUCCESS);
    Serial.println("RX and TX successfully initialized!");
    BlinkPattern(ProgramState::INIT_SUCCESSFUL);

    return 0;
}

int32_t shared_resources::init_radio(Cosmos::Devices::Radios::Astrodev &astrodev, HardwareSerial* hw_serial, uint32_t baud_rate, uint32_t tx_freq, uint32_t rx_freq)
{
    int32_t iretn = astrodev.Init(hw_serial, baud_rate);
    if (iretn < 0)
    {
        Serial.println("Error initializing Astrodev radio. Exiting...");
        return -1;
    }

    return connect_radio(astrodev, tx_freq, rx_freq);
}

int32_t shared_resources::connect_radio(Cosmos::Devices::Radios::Astrodev &astrodev, uint32_t tx_freq, uint32_t rx_freq)
{
    int32_t iretn = astrodev.Connect();
    if (iretn < 0)
    {
        Serial.println("Error connecting to Astrodev radio.");
        return -1;
    }
    astrodev.tcv_configuration.interface_baud_rate = 0;
    astrodev.tcv_configuration.power_amp_level = 140;
    astrodev.tcv_configuration.rx_baud_rate = 1;
    astrodev.tcv_configuration.tx_baud_rate = 1;
    astrodev.tcv_configuration.ax25_preamble_length = 20;
    astrodev.tcv_configuration.ax25_postamble_length = 20;
    astrodev.tcv_configuration.rx_modulation = Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
    astrodev.tcv_configuration.tx_modulation = Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK;
    astrodev.tcv_configuration.tx_frequency = tx_freq;
    astrodev.tcv_configuration.rx_frequency = rx_freq;
    memcpy(astrodev.tcv_configuration.ax25_source, "SOURCE", 6);
    memcpy(astrodev.tcv_configuration.ax25_destination, "DESTIN", 6);
    memset(&astrodev.tcv_configuration.config1, 0, 2);
    memset(&astrodev.tcv_configuration.config2, 0, 2);

    int8_t retries = RADIO_INIT_CONNECT_ATTEMPTS;

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

    retries = RADIO_INIT_CONNECT_ATTEMPTS;
    while ((iretn = astrodev.GetTCVConfig()) < 0)
    {
        Serial.println("Failed to gettcvconfig astrodev");
        if (--retries < 0)
        {
            return iretn;
        }
        threads.delay(5000);
    }
    Serial.print("Checking config settings... ");
    if (astrodev.tcv_configuration.interface_baud_rate != 0 ||
    astrodev.tcv_configuration.power_amp_level != 140 ||
    astrodev.tcv_configuration.rx_baud_rate != 1 ||
    astrodev.tcv_configuration.tx_baud_rate != 1 ||
    astrodev.tcv_configuration.ax25_preamble_length != 20||
    astrodev.tcv_configuration.ax25_postamble_length != 20 ||
    astrodev.tcv_configuration.rx_modulation != Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK ||
    astrodev.tcv_configuration.tx_modulation != Cosmos::Devices::Radios::Astrodev::Modulation::ASTRODEV_MODULATION_GFSK ||
    astrodev.tcv_configuration.tx_frequency != tx_freq ||
    astrodev.tcv_configuration.rx_frequency != rx_freq) {
        Serial.println("config mismatch detected!");
        return -1;
    }
    Serial.println("config check OK!");
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
  if (queue.size() > MAX_QUEUE_SIZE)
  {
      queue.pop_front();
  }
  queue.push_back(packet);
}

void shared_resources::set_radios_initialized_state(bool state)
{
    radios_initialized = state;
}

bool shared_resources::get_radios_initialized_state()
{
    return radios_initialized;
}

void shared_resources::set_radios_threads_started(bool state)
{
    radio_threads_started = state;
}

bool shared_resources::get_radios_threads_started()
{
    return radio_threads_started;
}
