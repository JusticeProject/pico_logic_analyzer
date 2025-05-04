#include <stdio.h>
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

#include "logic_analyzer.h"

//************************************************************************************************************

// variables only used in this file
static PIO pio;
static int sm = -1;
static int offset = -1;
static int dma_channel = -1;
static pio_program_t program;

//************************************************************************************************************

bool logic_analyzer_init(uint pin_base)
{
    // disable pull-up and pull-down on gpio pin
    // TODO: need to handle multiple pins
    gpio_disable_pulls(pin_base);
    
    // TODO: need to handle multiple pins
    uint16_t capture_prog_instr = pio_encode_in(pio_pins, 1);
    program.instructions = &capture_prog_instr;
    program.length = 1;
    program.origin = -1;

    // TODO: need to handle multiple pins
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&program, &pio, &sm, &offset, pin_base, 1, true);
    if (!success)
    {
        printf("could not init PIO\n");
        return false;
    }

    // Configure state machine to loop over this `in` instruction forever with autopush enabled.
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, pin_base);
    // TODO: need to handle multiple pins
    pio_sm_set_consecutive_pindirs(pio, sm, pin_base, 1, false);
    pio_gpio_init(pio, pin_base);
    sm_config_set_wrap(&c, offset, offset);
    // TODO: need to handle clock divider
    sm_config_set_clkdiv(&c, 1.0f);
    // Note that we may push at a < 32 bit threshold if pin_count does not
    // divide 32. We are using shift-to-right, so the sample data ends up
    // left-justified in the FIFO in this case, with some zeroes at the LSBs.
    // TODO: need to handle multiple pins
    sm_config_set_in_shift(&c, true, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio, sm, offset, &c);

    return true;
}

//************************************************************************************************************

void logic_analyzer_start(uint32_t* buffer, int capture_size_words, uint trigger_pin)
{
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);

    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

    dma_channel_configure(dma_channel, &c,
        buffer,        // Destination pointer
        &pio->rxf[sm],      // Source pointer
        capture_size_words, // Number of transfers
        true                // Start immediately, but it won't actually transfer the data until the PIO pushes data into the Rx FIFO
    );

    // TODO: allow configurable rising/falling edge
    pio_sm_exec(pio, sm, pio_encode_wait_gpio(false, trigger_pin));
    pio_sm_set_enabled(pio, sm, true);
}

//************************************************************************************************************

void logic_analyzer_wait_for_complete()
{
    dma_channel_wait_for_finish_blocking(dma_channel);
    dma_channel_cleanup(dma_channel);
    dma_channel_unclaim(dma_channel);
}

//************************************************************************************************************

void logic_analyzer_cleanup()
{
    pio_remove_program_and_unclaim_sm(&program, pio, sm, offset);
}
