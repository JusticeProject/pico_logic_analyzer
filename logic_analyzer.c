#include <stdio.h>
#include "hardware/clocks.h"

#include "logic_analyzer.h"

// import the assembled PIO state machine program
#include "logic_analyzer.pio.h"

//************************************************************************************************************

void logic_analyzer_init(PIO pio, int* p_sm, int* p_offset, uint pin_num)
{
    // disable pull-up and pull-down on gpio pin
    gpio_disable_pulls(pin_num);

    // install the program in the PIO shared instruction space
    if (pio_can_add_program(pio, &logic_analyzer_program))
    {
        *p_offset = pio_add_program(pio, &logic_analyzer_program);
    }
    else
    {
        *p_sm = -1;
        return;      // the program could not be added
    }

    // claim an unused state machine on this PIO
    *p_sm = pio_claim_unused_sm(pio, true);
    if (*p_sm == -1)
    {
        return;      // we were unable to claim a state machine
    }

    ///////////////////////////////////////////////////////////////////////////////////

    // Set the GPIO function of the pin (connect the PIO to the pad)
    pio_gpio_init(pio, pin_num);

    // Set the pin direction to `input` at the PIO
    pio_sm_set_consecutive_pindirs(pio, *p_sm, pin_num, 1, false);

    // Create a new state machine configuration
    pio_sm_config c = logic_analyzer_program_get_default_config(*p_offset);

    // configure the Input Shift Register
    sm_config_set_in_shift (&c,
                            true,       // shift right
                            true,       // enable autopush
                            32);        // autopush after 32 bits

    // join the FIFOs to make a single large receive FIFO
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

    // Map the IN pin group to one pin, namely the `pin`
    // parameter to this function.
    sm_config_set_in_pins(&c, pin_num);

    // Set the clock divider to 10 ticks per 562.5us burst period
    //float div = clock_get_hz(clk_sys) / (10.0 / 562.5e-6);
    float div = 1.0f;
    sm_config_set_clkdiv(&c, div);

    // Apply the configuration to the state machine
    pio_sm_init(pio, *p_sm, *p_offset, &c);

    // Set the state machine running
    pio_sm_set_enabled(pio, *p_sm, true);
}

//************************************************************************************************************

void logic_analyzer_restart(PIO pio, int sm, int offset)
{
    uint8_t pc = pio_sm_get_pc(pio, sm);
    printf("Restarting PIO, current pc = %u\n", pc);
    printf("Will use sm = %d offset = %d\n", sm, offset);

    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);

    pio_sm_restart(pio, sm);

    // jump to the beginning of the program when we restart
    pio_sm_exec(pio, sm, pio_encode_jmp(offset + logic_analyzer_offset_program_start));

    pio_sm_set_enabled(pio, sm, true);
    printf("Done restarting the PIO\n");

    pc = pio_sm_get_pc(pio, sm);
    printf("PIO pc = %u\n", pc);
}