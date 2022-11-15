#ifndef _TEST_BLINKER_H
#define _TEST_BLINKER_H

// This has helper functions for making the teensy blink
// in certain patterns to visually verify the state of the program.

#include <Arduino.h>
#include <TeensyThreads.h>

namespace Lithium3
{
    enum class ProgramState {
        RADIO_RETRY_ATTEMPT,
        // RXS
        RADIO_RXS_ATTEMPT_INIT,
        RADIO_RXS_INIT_SUCCESS,
        RADIO_RXS_INIT_FAIL,
        // TXS
        RADIO_TXS_ATTEMPT_INIT,
        RADIO_TXS_INIT_SUCCESS,
        RADIO_TXS_INIT_FAIL,
        // Final verdict
        INIT_SUCCESSFUL,
        INIT_FAIL,
    };
    // Blink the light in a predefined pattern according to program state
    void BlinkPattern(ProgramState state);
    // Blink the light on then off for specified number of milliseconds
    void Blink(uint32_t on_ms, uint32_t off_ms);
}

#endif
