#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "nec_8.h"

// import the assembled PIO state machine program
#include "nec_8.pio.h"

//************************************************************************************************************

static inline void nec_8_program_init (PIO pio, uint sm, uint offset, uint pin)
{
    // Set the pin direction to `input` at the PIO
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);

    // Create a new state machine configuration
    pio_sm_config c = nec_8_program_get_default_config(offset);

    // configure the Input Shift Register
    sm_config_set_in_shift (&c,
                            true,       // shift right
                            true,       // enable autopush
                            32);        // autopush after 32 bits

    // join the FIFOs to make a single large receive FIFO
    sm_config_set_fifo_join (&c, PIO_FIFO_JOIN_RX);

    // Map the IN pin group to one pin, namely the `pin`
    // parameter to this function.
    sm_config_set_in_pins (&c, pin);

    // Map the JMP pin to the `pin` parameter of this function.
    sm_config_set_jmp_pin (&c, pin);

    // Set the clock divider to 10 ticks per 562.5us burst period
    float div = clock_get_hz (clk_sys) / (10.0 / 562.5e-6);
    sm_config_set_clkdiv (&c, div);

    // Apply the configuration to the state machine
    pio_sm_init (pio, sm, offset, &c);

    // Set the state machine running
    pio_sm_set_enabled (pio, sm, true);
}

//************************************************************************************************************

// Claim an unused state machine on the specified PIO and configure it
// to receive NEC IR frames on the given GPIO pin.
//
// Returns: the state machine number on success, otherwise -1
int nec_8_init(PIO pio, uint pin_num)
{
    // install the program in the PIO shared instruction space
    uint offset;
    if (pio_can_add_program(pio, &nec_8_program))
    {
        offset = pio_add_program(pio, &nec_8_program);
    }
    else
    {
        return -1;      // the program could not be added
    }

    // claim an unused state machine on this PIO
    int sm = pio_claim_unused_sm(pio, true);
    if (sm == -1)
    {
        return -1;      // we were unable to claim a state machine
    }

    // configure and enable the state machine
    nec_8_program_init(pio, sm, offset, pin_num);

    return sm;
}

//************************************************************************************************************

// Validate a 32-bit frame and store the address and data at the locations
// provided.
//
// Returns: `true` if the frame was valid, otherwise `false`
bool nec_decode_frame(uint32_t frame, uint8_t* p_address, uint8_t* p_data) {

    // access the frame data as four 8-bit fields
    union {
        uint32_t raw;
        struct {
            uint8_t address;
            uint8_t inverted_address;
            uint8_t data;
            uint8_t inverted_data;
        };
    } f;

    f.raw = frame;

    // a valid (non-extended) 'NEC' frame should contain 8 bit
    // address, inverted address, data and inverted data
    if (f.address != (f.inverted_address ^ 0xff) ||
        f.data != (f.inverted_data ^ 0xff)) {
        return false;
    }

    // store the validated address and data
    *p_address = f.address;
    *p_data = f.data;

    return true;
}
