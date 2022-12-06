#include "helpers/TestBlinker.h"

// Length of single pulse in milliseconds
#define PULSE 100

void Lithium3::BlinkPattern(Lithium3::ProgramState state)
{
    switch(state)
    {
    // .
    case Lithium3::ProgramState::RADIO_RETRY_ATTEMPT:
    case Lithium3::ProgramState::RADIO_RXS_ATTEMPT_INIT:
        Blink(PULSE, PULSE);
        break;
    // . . .
    case Lithium3::ProgramState::RADIO_RXS_INIT_SUCCESS:
    case Lithium3::ProgramState::RADIO_TXS_INIT_SUCCESS:
        Blink(PULSE, PULSE);
        Blink(PULSE, PULSE);
        Blink(PULSE, PULSE);
        break;
    // --- --- ---
    case Lithium3::ProgramState::RADIO_RXS_INIT_FAIL:
    case Lithium3::ProgramState::RADIO_TXS_INIT_FAIL:
        Blink(PULSE*3, PULSE);
        Blink(PULSE*3, PULSE);
        Blink(PULSE*3, PULSE);
        break;
    // . .
    case Lithium3::ProgramState::RADIO_TXS_ATTEMPT_INIT:
        Blink(PULSE, PULSE);
        Blink(PULSE, PULSE);
        break;
    // . . . . . . . . . . .  forever
    case Lithium3::ProgramState::INIT_SUCCESSFUL:
        while (true)
        {
            Blink(PULSE, PULSE);
        }
        break;
    // ------------- flatline forever
    case Lithium3::ProgramState::INIT_FAIL:
        // On
        digitalWrite(LED_BUILTIN, HIGH);
        while (true)
        {
            threads.delay(1000);
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