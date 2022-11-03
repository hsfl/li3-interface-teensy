#include "astrodev.h"
// #include "support/timelib.h"
#include <elapsedMillis.h>

namespace Cosmos {
    namespace Devices {
        namespace Radios {
            Astrodev::Astrodev()
            {

            }

            int32_t Astrodev::Init(HardwareSerial* new_serial, uint32_t speed)
            {
                int32_t iretn = 0;
                int32_t timeout = 5;
                serial = new_serial;
                serial->begin(speed);
                serial->clear();
                serial->flush();

                Serial.println("Resetting radio...");
                // do
                // {
                //     iretn = Reset();
                //     threads.delay(2000);
                //     if (timeout-- <= 0)
                //     {
                //         Serial.println("Radio reset unsuccessful");
                //         return iretn;
                //     }
                // } while (iretn < 0);

                // Serial.println("Radio reset successful");
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

                char msg[4];

                if (message.header.sizelo > MTU)
                {
                    return GENERAL_ERROR_BAD_SIZE;
                }
                
                message.header.sync0 = SYNC0;
                message.header.sync1 = SYNC1;
                message.header.type = COMMAND;
                message.header.sizehi = 0;
                message.header.cs = CalcCS(&message.preamble[2], 4);
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
                serial->write(&message.preamble[0], 8);
                // Skip payload and payload crc if header-only
                if (message.header.sizehi == 0 && message.header.sizelo == 0)
                {
                    Serial.println();
                    // Important: don't take this delay out, radio doesn't work without it
                    //threads.delay(10);
                    return message.header.sizelo;
                }

                Serial.print(" | pay: ");

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
                    sprintf(msg, "0x%02X", message.payload[i]);
                    Serial.print(msg);
                    Serial.print(" ");
                }
                Serial.println();

                // Important: don't take this delay out, radio doesn't work without it
                // threads.delay(10);
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

                tmessage.header.command = (uint8_t)Command::TRANSMIT;
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
                int32_t iretn;
                frame message;

                message.header.command = (uint8_t)Command::NOOP;
                message.header.sizehi = 0;
                message.header.sizelo = 0;
                iretn = Transmit(message);
                Serial.print("Transmit iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                threads.delay(10);
                iretn = Receive(message);
                Serial.print("Receive iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                return 0;
            }

            int32_t Astrodev::Reset()
            {
                int32_t iretn;
                frame message;

                message.header.command = (uint8_t)Command::RESET;
                message.header.sizehi = 0;
                message.header.sizelo = 0;
                iretn = Transmit(message);
                Serial.print("Transmit iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                threads.delay(1000);
                iretn = Receive(message);
                Serial.print("Receive iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                return iretn;
            }

            // int32_t Astrodev::SendData(vector<uint8_t> data)
            // {
            //     int32_t iretn;
            //     frame message;

            //     if (data.size() > MTU)
            //     {
            //         return GENERAL_ERROR_BAD_SIZE;
            //     }
            //     message.header.command = (uint8_t)Command::RESET;
            //     message.header.size = data.size();
            //     memcpy(message.payload, &data[0], data.size());
            //     iretn = Transmit(message);
            //     return iretn;
            // }

            int32_t Astrodev::GetTCVConfig()
            {
                int32_t iretn;
                frame message;

                message.header.command = (uint8_t)Command::GETTCVCONFIG;
                message.header.sizehi = 0;
                message.header.sizelo = 0;
                iretn = Transmit(message);
                Serial.print("Transmit iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                threads.delay(10);
                iretn = Receive(message);
                Serial.print("Receive iretn: ");
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
                int32_t iretn;
                frame message;

                message.header.command = (uint8_t)Command::SETTCVCONFIG;
                message.header.sizehi = 0;
                message.header.sizelo = sizeof(tcv_configuration);
                message.tcv = tcv_configuration;
                iretn = Transmit(message);
                Serial.print("Transmit iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                // Wait at least 250 ms for settings to be applied
                // But seems to require about 10x as long
                threads.delay(2500);
                iretn = Receive(message);
                Serial.print("Receive iretn: ");
                Serial.println(iretn);
                if (iretn < 0)
                {
                    return iretn;
                }
                return 0;
            }

            // int32_t Astrodev::GetTelemetry()
            // {
            //     int32_t iretn;
            //     frame message;

            //     message.header.command = (uint8_t)Command::TELEMETRY;
            //     message.header.size = 0;
            //     iretn = Transmit(message);
            //     return iretn;
            // }

            int32_t Astrodev::SetRFConfig(rf_config config)
            {
                int32_t iretn;
                frame message;

                message.header.command = (uint8_t)Command::RFCONFIG;
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

            int32_t Astrodev::Receive(frame &message)
            {
                int16_t ch = -1;
                
                union
                {
                    uint16_t cs;
                    uint8_t csb[2];
                };
                uint16_t size;

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
                    return ASTRODEV_ERROR_SYNC1;
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
                cs = CalcCS(&message.preamble[2], 4);
                if (cs != message.header.cs)
                {
                    return (ASTRODEV_ERROR_HEADER_CS);
                }

                Serial.print("header.command: ");
                Serial.println(message.header.command);

                // Check payload bytes
                // If 0a 0a, it's an acknowledge response
                // If FF FF, it's a NOACK response
                // Otherwise, it's a response with a payload in it
                if (message.header.status.ack == 0x0a && message.header.sizelo == 0x0a)
                {
                    buffer_full = message.header.status.buffer_full;
                    // if (buffer_full)
                    // {
                    //     Serial.println("BUFFER FULL!!!!!!!!!!!!!!!!!!!!!!!!!");
                    // }
                    last_ack = true;
                    return 0;
                }
                else if (message.header.status.ack == 0x0f && message.header.sizelo == 0xff)
                {
                    buffer_full = message.header.status.buffer_full;
                    // if (buffer_full)
                    // {
                    //     Serial.println("BUFFER FULL##############################");
                    // }
                    last_ack = false;
                    return ASTRODEV_ERROR_NACK;
                }

                // Read rest of payload bytes
                csb[0] = message.header.sizelo;
                csb[1] = message.header.sizehi;
                size = cs;
                size_t sizeToRead = size+2;
                Serial.print("size: ");
                Serial.println(cs);
                size_t readLocation = 0;
                bytesRead = serial->readBytes(&message.payload[readLocation], sizeToRead);
                Serial.print("bytesRead: ");
                Serial.println(bytesRead);
                uint8_t iterations = 0;
                // Read in remaining bytes, if any
                while (bytesRead != sizeToRead)
                {
                    sizeToRead -= bytesRead;
                    readLocation += bytesRead;
                    threads.delay(500);
                    bytesRead = serial->readBytes(&message.payload[readLocation], sizeToRead);
                Serial.print("bytesRead: ");
                Serial.println(bytesRead);
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

                // Handle payload
                switch ((Command)message.header.command)
                {
                case Command::GETTCVCONFIG:
                    Serial.println("in get tcv config");
                    break;
                case Command::RECEIVE:
                    Serial.println("in receive");
                    // vector<uint8_t> payload;
                    // payload.insert(payload.begin(), (uint8_t *)message.payload, (uint8_t *)message.payload+message.header.sizelo);
                    // push_queue_in(payload);
            //        printf("Payload %u %u %u %u\n", handle.payloads.size(), payload[0], payload[16], payload[payload.size()-5]);
                    break;
                case Command::TELEMETRY:
                    Serial.println("in telem");
                    //message.telem = message..telemetry;
            //        printf("Telemetry Ops %hu Temp %hd Time %u RSSI %hu RXbytes %u TXbytes %u\n",
            //               frame.telemetry.op_counter,
            //               frame.telemetry.msp430_temp,
            //               frame.telemetry.time_count,
            //               frame.telemetry.rssi,
            //               frame.telemetry.bytes_rx,frame.telemetry.bytes_tx);
                    break;
                default:
                    break;
                }

                char msg[4];
                Serial.print("recv msg: ");
                for (uint16_t i=0; i<message.header.sizelo; i++)
                {
                    sprintf(msg, "0x%02X", message.payload[i]);
                    Serial.print(msg);
                    Serial.print(" ");
                }
                Serial.println();

                return (message.header.command);
            }

            // void Astrodev::receive_loop()
            // {
            //     running = true;
            //     elapsedMillis wdt;

            //     last_error = 0;
            //     while(running)
            //     {
            //         int32_t ch;
            //         frame next_frame;
            //         last_error = 0;
            //         while (wdt < 5000)
            //         {
            //             // Wait for first sync character
            //             do
            //             {
            //                 ch = serial->read();
            //                 if (ch < 0)
            //                 {
            //                     last_error = ASTRODEV_ERROR_SYNC0;
            //                     break;
            //                 }
            //             } while (ch != SYNC0);
            //             next_message.preamble[0] = ch;
            //             wdt = 0;

            //             // Second sync character must immediately follow
            //             ch = serial->read();
            //             if (ch < 0 || ch != SYNC1)
            //             {
            //                 last_error = ASTRODEV_ERROR_SYNC1;
            //                 break;
            //             }
            //             next_message.preamble[1] = ch;
            //             wdt = 0;

            //             // Read rest of header
            //             for (uint16_t i=2; i<8; ++i)
            //             {
            //                 ch = serial->read();
            //                 if (ch < 0)
            //                 {
            //                     last_error = ASTRODEV_ERROR_HEADER;
            //                     break;
            //                 }
            //                 next_message.preamble[i] = ch;
            //             }
            //             if (last_error < 0)
            //             {
            //                 break;
            //             }
            //             wdt = 0;

            //             // Check header for accuracy
            //             union
            //             {
            //                 uint16_t cs;
            //                 uint8_t csb[2];
            //             };
            //             cs = CalcCS(&next_message.preamble[2], 4);
            //             if (cs != next_message.header.cs)
            //             {
            //                 last_error = ASTRODEV_ERROR_HEADER_CS;
            //                 break;
            //             }
            //             wdt = 0;

            //             // Check for NOACK
            //             if (next_message.header.ack == 0x0a && next_message.header.size == 0x0a)
            //             {
            //                 last_ack = true;
            //                 last_command = (Command)next_message.header.command;
            //                 wdt = 0;
            //             }
            //             else if (next_message.header.ack == 0x0f && next_message.header.size == 0xff)
            //             {
            //                 last_ack = false;
            //                 last_command = (Command)next_message.header.command;
            //                 wdt = 0;
            //             }
            //             else
            //             {
            //                 // Read rest of frame
            //                 switch ((Command)next_message.header.command)
            //                 {
            //                 // Simple ACK:NOACK
            //                 case Command::NOOP:
            //                 case Command::RESET:
            //                 case Command::TRANSMIT:
            //                 case Command::SETTCVCONFIG:
            //                 case Command::FLASH:
            //                 case Command::RFCONFIG:
            //                 case Command::BEACONDATA:
            //                 case Command::BEACONCONFIG:
            //                 case Command::DIOKEY:
            //                 case Command::FIRMWAREUPDATE:
            //                 case Command::FIRMWAREPACKET:
            //                 case Command::FASTSETPA:
            //                     break;
            //                 default:
            //                     {
            //                         for (uint16_t i=0; i<next_message.header.size+2; ++i)
            //                         {
            //                             if((ch = serial->read()) < 0)
            //                             {
            //                                 //                                        serial->drain();
            //                                 last_error = ASTRODEV_ERROR_PAYLOAD;
            //                                 break;
            //                             }
            //                             next_frame.payload[i] = ch;
            //                         }
            //                         if (last_error < 0)
            //                         {
            //                             break;
            //                         }

            //                         // Check payload for accuracy
            //                         cs = next_frame.payload[next_message.header.size] | (next_frame.payload[next_message.header.size+1] << 8L);
            //                         if (cs != CalcCS(&next_message.preamble[2], 6+next_message.header.size))
            //                         {
            //                             last_error = ASTRODEV_ERROR_PAYLOAD_CS;
            //                             break;
            //                         }
            //                     }
            //                     break;
            //                 }
            //                 if (next_message.header.command == (uint8_t)Command::TELEMETRY)
            //                 {
            //                     last_telem = next_frame.telem;
            //                 }
            //                 else if (next_message.header.command == (uint8_t)Command::RECEIVE)
            //                 {
            //                     vector<uint8_t> payload;
            //                     payload.insert(payload.begin(), (uint8_t *)next_frame.payload, (uint8_t *)next_frame.payload+next_message.header.size);
            //                     push_queue_in(payload);
            //                 }
            //             }
            //         }
            //     }
            // }

            int32_t Astrodev::push_queue_in(vector<uint8_t> data)
            {
                Threads::Scope lock(qmutex_in);
                queue_in.push(data);
                return 1;
            }

        }
    }
}
