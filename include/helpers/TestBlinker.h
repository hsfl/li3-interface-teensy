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
        // RX
        RADIO_RX_ATTEMPT_INIT,
        RADIO_RX_INIT_SUCCESS,
        RADIO_RX_INIT_FAIL,
        // TX
        RADIO_TX_ATTEMPT_INIT,
        RADIO_TX_INIT_SUCCESS,
        RADIO_TX_INIT_FAIL,
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
