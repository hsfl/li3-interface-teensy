#include "helpers/TestBlinker.h"

// Length of single pulse in milliseconds
#define PULSE 100

void Lithium3::BlinkPattern(Lithium3::ProgramState state)
{
    switch(state)
    {
    // .
    case Lithium3::ProgramState::RADIO_RETRY_ATTEMPT:
    case Lithium3::ProgramState::RADIO_RX_ATTEMPT_INIT:
        Blink(PULSE, PULSE);
        break;
    // . . .
    case Lithium3::ProgramState::RADIO_RX_INIT_SUCCESS:
    case Lithium3::ProgramState::RADIO_TX_INIT_SUCCESS:
        Blink(PULSE, PULSE);
        Blink(PULSE, PULSE);
        Blink(PULSE, PULSE);
        break;
    // --- --- ---
    case Lithium3::ProgramState::RADIO_RX_INIT_FAIL:
    case Lithium3::ProgramState::RADIO_TX_INIT_FAIL:
        Blink(PULSE*3, PULSE);
        Blink(PULSE*3, PULSE);
        Blink(PULSE*3, PULSE);
        break;
    // . .
    case Lithium3::ProgramState::RADIO_TX_ATTEMPT_INIT:
        Blink(PULSE, PULSE);
        Blink(PULSE, PULSE);
        break;
    // . . . . . . . . . . .  for 5 seconds
    case Lithium3::ProgramState::INIT_SUCCESSFUL:
        {
            elapsedMillis timeout;
            while (timeout < 5000)
            {
                Blink(PULSE, PULSE);
            }
        }
        break;
    // Flatline for 5 seconds
    case Lithium3::ProgramState::INIT_FAIL:
        // On
        {
            elapsedMillis timeout;
            digitalWrite(LED_BUILTIN, HIGH);
            while (timeout < 5000)
            {
                threads.delay(1000);
            }
        }
        break;
    default:
        break;
    }
    threads.delay(PULSE*10);
    return;
}

void Lithium3::Blink(uint32_t on_ms, uint32_t off_ms)
{
    // On
    digitalWrite(LED_BUILTIN, HIGH);
    threads.delay(on_ms);
    // Off
    digitalWrite(LED_BUILTIN, LOW);
    threads.delay(off_ms);
    return;
}