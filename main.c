#include <stdio.h>
#include "pico/stdlib.h"

#include "logic_analyzer.h"
#include "nec_8.h"

//************************************************************************************************************

#define CAPTURE_GPIO_PIN_BASE 0

// global variables
int nec_8_sm;

//************************************************************************************************************

void nec_available_handler()
{
    // display any frames received in the NEC FIFO
    if (pio_sm_is_rx_fifo_empty(pio0, nec_8_sm))
    {
        printf("?? NEC Rx FIFO is empty\n");
    }
    else
    {
        uint32_t rx_frame = pio_sm_get(pio0, nec_8_sm);

        uint8_t address, nec_8_data;
        if (nec_decode_frame(rx_frame, &address, &nec_8_data))
        {
            // normally shouldn't do printf inside an interrupt handler...but it's not critical in this case, it's only for debug
            printf("received address= %02x, data = %02x\n", address, nec_8_data);
        }
        else
        {
            printf("received: %08x\n", rx_frame);
        }
    }
}

//************************************************************************************************************

int main() {
    stdio_init_all();
    
    // configure and enable the state machines
    int logic_analyzer_sm;
    int logic_analyzer_offset;
    logic_analyzer_init(pio0, &logic_analyzer_sm, &logic_analyzer_offset, CAPTURE_GPIO_PIN_BASE);
    nec_8_sm = nec_8_init(pio0, CAPTURE_GPIO_PIN_BASE);

    if (nec_8_sm == -1 || logic_analyzer_sm == -1 || logic_analyzer_offset == -1)
    {
        while (true)
        {
            printf("could not configure PIO\n");
            sleep_ms(1000);
        }
    }

    bool nec_decoding_on = false;
    
    while (true)
    {
        int c = getchar_timeout_us(0);
        if ('m' == c)
        {
            // TODO: remove this now that interrupts are working?
            // manual query for NEC data
            if (!pio_sm_is_rx_fifo_empty(pio0, nec_8_sm))
            {
                uint32_t rx_frame = pio_sm_get(pio0, nec_8_sm);
                printf("0x%x\n", rx_frame);
            }
            else
            {
                printf("No NEC data\n");
            }
        }
        else if ('n' == c)
        {
            // turn the interrupt on/off for automatic decoding of new NEC data
            nec_decoding_on = !nec_decoding_on;
            notify_new_nec_data(pio0, nec_8_sm, nec_available_handler, nec_decoding_on);
        }
        else if ('q' == c)
        {
            if (pio_sm_is_rx_fifo_empty(pio0, logic_analyzer_sm))
            {
                printf("No data in logic analyzer FIFO\n");
            }
            else
            {
                uint32_t data = pio_sm_get(pio0, logic_analyzer_sm);
                printf("0x%08x\n", data);
            }
        }
        else if ('r' == c)
        {
            logic_analyzer_restart(pio0, logic_analyzer_sm, logic_analyzer_offset);
        }
        else if ('c' == c)
        {
            printf("Starting capture\n");
            // TODO: do DMA
        }

        // TODO: set timescale with pio_sm_set_clkdiv
    }
}
