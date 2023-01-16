#include "astrodev.h"
// #include "support/timelib.h"
#include <elapsedMillis.h>

namespace Cosmos {
    namespace Devices {
        namespace Radios {
            Astrodev::Astrodev()
            {
                buffer_full = false;
            }

            Astrodev::Astrodev(HardwareSerial* hw_serial) : serial(hw_serial)
            {
                buffer_full = false;
            }

            int32_t Astrodev::Init(HardwareSerial* hw_serial, uint32_t baud_rate)
            {
                serial = hw_serial;
                serial->begin(baud_rate);
                serial->clear();
                serial->flush();

                return 0;
            }

            int32_t Astrodev::Connect()
            {
                int32_t iretn = 0;
                int32_t retries = 5;
                Serial.println("Connecting to radio, resetting...");
                do
                {
                    iretn = Reset();
                    threads.delay(2000);
                    if (--retries < 0)
                    {
                        Serial.println("Radio reset unsuccessful");
                        return iretn;
                    }
                } while (iretn < 0);

                Serial.println("Radio reset successful");
                Serial.println("Radio ping test...");

                if ((iretn=Ping()) < 0)
                {
                    Reset();
                    threads.delay(5000);
                    if ((iretn=Ping()) < 0)
                    {
                        return iretn;
                    }
                }
                Serial.println("Radio ping test successful");
                Serial.println("Astrodev radio Init success.");

                return 0;
            }

            int32_t Astrodev::Transmit(frame &message)
            {
                Threads::Scope lock(qmutex_out);

                if (message.header.sizelo > MTU)
                {
                    return GENERAL_ERROR_BAD_SIZE;
                }

                switch (message.header.command)
                {
                case Command::TRANSMIT:
                    {
                        ack_transmit = false;
                    }
                    break;
                default:
                    break;
                }
                
                message.header.sync0 = SYNC0;
                message.header.sync1 = SYNC1;
                message.header.type = COMMAND;
                message.header.sizehi = 0;
                message.header.cs = CalcCS(&message.preamble[2], 4);
#ifdef DEBUG_PRINT
                char msg[4];
                Serial.print("tpre: ");
                for (uint16_t i=0; i<8; i++)
                {
                    //serial->write(message.preamble[i]);
                    sprintf(msg, "0x%02X", message.preamble[i]);
                    Serial.print(msg);
                    Serial.print(" ");
                    if (i == 6)
                    {
                        //threads.delay(50);
                    }
                }
#endif
                serial->write(&message.preamble[0], 8);
                // Skip payload and payload crc if header-only
                if (message.header.sizehi == 0 && message.header.sizelo == 0)
                {
#ifdef DEBUG_PRINT
                    Serial.println();
#endif
                    return message.header.sizelo;
                }
#ifdef DEBUG_PRINT
                Serial.print(" | pay: ");
#endif

                union
                {
                    uint16_t cs;
                    uint8_t csb[2];
                };
                cs = CalcCS(&message.preamble[2], 6+message.header.sizelo);
                message.payload[message.header.sizelo] = csb[0];
                message.payload[message.header.sizelo+1] = csb[1];

                for (uint16_t i=0; i<message.header.sizelo+2; i++)
                {
                    serial->write(message.payload[i]);
#ifdef DEBUG_PRINT
                    sprintf(msg, "0x%02X", message.payload[i]);
                    Serial.print(msg);
                    Serial.print(" ");
#endif
                }
#ifdef DEBUG_PRINT
                Serial.println();
#endif

                // 10 = 8 bytes header + 2 byte crc
                return message.header.sizelo + 10;
            }

            // Send out a PacketComm packet
            int32_t Astrodev::Transmit(Cosmos::Support::PacketComm &packet)
            {
                // Apply packetcomm protocol wrapping
                int32_t iretn = packet.Wrap();
                if (iretn < 0)
                {
                    return iretn;
                }

                tmessage.header.command = Command::TRANSMIT;
                tmessage.header.sizelo = packet.wrapped.size();
                if (tmessage.header.sizelo > MTU)
                {
                    return GENERAL_ERROR_BAD_SIZE;
                }

                memcpy(&tmessage.payload[0], packet.wrapped.data(), packet.wrapped.size());
                
                return Transmit(tmessage);
            }

            int32_t Astrodev::Ping()
            {
                return Ping(true);
            }

            int32_t Astrodev::Ping(bool get_response)
            {
                int32_t iretn;
                frame message;

                message.header.command = Command::NOOP;
                message.header.sizehi = 0;
                message.header.sizelo = 0;
                iretn = Transmit(message);
#ifdef DEBUG_PRINT
                Serial.print("PTransmit iretn: ");
                Serial.println(iretn);
#endif
                if (iretn < 0)
                {
                    return iretn;
                }

                // Don't try to get the response
                if (!get_response)
                {
                    return iretn;
                }

                threads.delay(10);
                iretn = Receive(message);
#ifdef DEBUG_PRINT
                Serial.print("PReceive iretn: ");
                Serial.println(iretn);
#endif
                if (iretn < 0)
                {
                    return iretn;
                }
                return 0;
            }

            int32_t Astrodev::Reset()
            {
                return Reset(true);
            }

            int32_t Astrodev::Reset(bool get_response)
            {
                int32_t iretn;
                frame message;

                message.header.command = Command::RESET;
                message.header.sizehi = 0;
                message.header.sizelo = 0;
                iretn = Transmit(message);
#ifdef DEBUG_PRINT
                Serial.print("RTransmit iretn: ");
                Serial.println(iretn);
#endif
                if (iretn < 0)
                {
                    return iretn;
                }
                // Don't try to get the response
                if (!get_response)
                {
                    return iretn;
                }

                threads.delay(1000);
                iretn = Receive(message);
                Serial.print("RReceive iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                return iretn;
            }

            int32_t Astrodev::GetTCVConfig()
            {
                return GetTCVConfig(true);
            }

            int32_t Astrodev::GetTCVConfig(bool get_response)
            {
                int32_t iretn;
                frame message;

                message.header.command = Command::GETTCVCONFIG;
                message.header.sizehi = 0;
                message.header.sizelo = 0;
                iretn = Transmit(message);
#ifdef DEBUG_PRINT
                Serial.print("GTransmit iretn: ");
                Serial.println(iretn);
#endif
                if (iretn < 0)
                {
                    return iretn;
                }
                
                // Don't try to get the response
                if (!get_response)
                {
                    return iretn;
                }

                threads.delay(10);
                iretn = Receive(message);
                Serial.print("GReceive iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                memcpy(&tcv_configuration, &message.payload[0], message.header.sizelo);
                return 0;
            }

            int32_t Astrodev::SetTCVConfig()
            {
                return SetTCVConfig(true);
            }

            int32_t Astrodev::SetTCVConfig(bool get_response)
            {
                int32_t iretn;
                frame message;

                message.header.command = Command::SETTCVCONFIG;
                message.header.sizehi = 0;
                message.header.sizelo = sizeof(tcv_configuration);
                message.tcv = tcv_configuration;
                iretn = Transmit(message);
#ifdef DEBUG_PRINT
                Serial.print("STransmit iretn: ");
                Serial.println(iretn);
#endif
                if (iretn < 0)
                {
                    return iretn;
                }

                // Don't try to get the response
                if (!get_response)
                {
                    return iretn;
                }

                // Wait at least 250 ms for settings to be applied
                // But seems to require about 10x as long
                threads.delay(2500);
                iretn = Receive(message);
                Serial.print("SReceive iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                return 0;
            }

            int32_t Astrodev::GetTelemetry()
            {
                int32_t iretn;
                frame message;

                message.header.command = Command::TELEMETRY;
                message.header.sizehi = 0;
                message.header.sizelo = 0;
                iretn = Transmit(message);
#ifdef DEBUG_PRINT
                Serial.print("TTransmit iretn: ");
                Serial.println(iretn);
#endif
                if (iretn < 0)
                {
                    return iretn;
                }

                return 0;
            }

            int32_t Astrodev::SetRFConfig(rf_config config)
            {
                int32_t iretn;
                frame message;

                message.header.command = Command::RFCONFIG;
                message.header.sizehi = 0;
                message.header.sizelo = sizeof(rf_config);
                message.rf = config;
                iretn = Transmit(message);
                // Wait at least 250 ms for settings to be applied
                threads.delay(500);
                return iretn;
            }


            uint16_t Astrodev::CalcCS(uint8_t *data, uint16_t size)
            {
                uint8_t ck_a=0, ck_b=0;
                uint16_t cs;

                for (uint16_t i=0; i<size; ++i)
                {
                    ck_a += data[i];
                    ck_b += ck_a;
                }
                cs = ck_a | (ck_b << 8L);

                return (cs);
            }

            // Returns negative on error, 0 on header-only acknowledge responses,
            // or Astrodev::Command enum type on a message with a payload
            int32_t Astrodev::Receive(frame &message)
            {
                Threads::Scope lock(qmutex_in);
                int16_t ch = -1;

                // Wait for sync characters
                elapsedMillis wdt;
                while (wdt < 1000)
                {
                    threads.delay(10);
                    ch = serial->read();
                    if (ch != SYNC0)
                    {
                        continue;
                    }
                    ch = serial->read();
                    if (ch == SYNC1)
                    {
                        break;
                    }
                }
                if (ch != SYNC1)
                {
                    return COSMOS_ASTRODEV_ERROR_SYNC1;
                }
                message.preamble[0] = SYNC0;
                message.preamble[1] = SYNC1;

                // Read rest of header
                size_t bytesRead = serial->readBytes(&message.preamble[2], 6);
                if (bytesRead < 6)
                {
                    // threads.yield();
                    // Read rest of header in case not all bytes were read
                    size_t bytesRead2 = serial->readBytes(&message.preamble[2+bytesRead], 6-bytesRead);
                    if (bytesRead + bytesRead2 != 6)
                    {
                        return ASTRODEV_ERROR_HEADER;
                    }
                }

                // Check header for accuracy
                uint16_t cs = CalcCS(&message.preamble[2], 4);
                if (cs != message.header.cs)
                {
                    return (ASTRODEV_ERROR_HEADER_CS);
                }

#ifdef DEBUG_PRINT
                Serial.print("header.command: ");
                Serial.println(unsigned(message.header.command));
#endif

                // Check payload bytes
                // If 0a 0a, it's an acknowledge response
                // If FF FF, it's a NOACK response
                // Otherwise, it's a response with a payload in it
                if (message.header.status.ack == 0x0a && message.header.sizelo == 0x0a)
                {
                    buffer_full = message.header.status.buffer_full;
#ifdef DEBUG_PRINT
                    if (buffer_full.load())
                    {
                        Serial.println("BUFFER FULL!!!!!!!!!!!!!!!!!!!!!!!!!");
                    }
                    else {
                        Serial.println("BUFFER OK!!!!!!!!!!!!!!!!!!!!!!!!!");
                    }
#endif
                    switch (message.header.command)
                    {
                    case Command::TRANSMIT:
                        {
                            ack_transmit = true;
                        }
                        break;
                    default:
                        break;
                    }
                    // TODO: check this doesn't catch any strange stuff
                    return (int32_t)message.header.command;
                }
                else if (message.header.status.ack == 0x0f && message.header.sizelo == 0xff)
                {
                    buffer_full = message.header.status.buffer_full;
#ifdef DEBUG_PRINT
                    if (buffer_full.load())
                    {
                        Serial.println("BUFFER FULL##############################");
                    }
                    else {
                        Serial.println("BUFFER OK##############################");
                    }
#endif
                    switch (message.header.command)
                    {
                    case Command::TRANSMIT:
                        {
                            ack_transmit = false;
                        }
                        break;
                    default:
                        break;
                    }
                    return ASTRODEV_ERROR_NACK;
                }

                // TODO: This !size check was put in to fake some radio command handling,
                // should double-check that it doesn't break anything.
                // If not an ack-type, then return if no payload
                uint8_t size = message.header.sizelo;
                if (!size)
                {
                    return (int32_t)message.header.command;
                }
                // Read rest of payload bytes
                size_t sizeToRead = size+2;
#ifdef DEBUG_PRINT
                Serial.print("size: ");
                Serial.println(size);
#endif
                size_t readLocation = 0;
                bytesRead = serial->readBytes(&message.payload[readLocation], sizeToRead);
#ifdef DEBUG_PRINT
                Serial.print("bytesRead: ");
                Serial.println(bytesRead);
#endif
                uint8_t iterations = 0;
                // Read in remaining bytes, if any
                while (bytesRead != sizeToRead)
                {
                    sizeToRead -= bytesRead;
                    readLocation += bytesRead;
                    threads.delay(500);
                    bytesRead = serial->readBytes(&message.payload[readLocation], sizeToRead);
#ifdef DEBUG_PRINT
                Serial.print("bytesRead: ");
                Serial.println(bytesRead);
#endif
                    if (++iterations > 20)
                    {
                        break;
                    }
                }

                // Check payload for accuracy
                cs = message.payload[size] | (message.payload[size+1] << 8L);
                if (cs != CalcCS(&message.preamble[2], 6+size))
                {
                    return (ASTRODEV_ERROR_PAYLOAD_CS);
                }
#ifdef DEBUG_PRINT
                char msg[4];
                Serial.print("recv msg: ");
                for (uint16_t i=0; i<message.header.sizelo; i++)
                {
                    sprintf(msg, "0x%02X", message.payload[i]);
                    Serial.print(msg);
                    Serial.print(" ");
                }
                Serial.println();
#endif
                // Handle command types outside of astrodev library
                // i.e., pushing payloads to appropriate queues and what not
                return (int32_t)message.header.command;
            }

            void Astrodev::setSerial(HardwareSerial* new_serial)
            {
                serial = new_serial;
            }
        }
    }
}
