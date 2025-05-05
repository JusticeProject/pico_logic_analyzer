#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"

#include "logic_analyzer.h"
#include "nec_8.h"

//************************************************************************************************************

#define CAPTURE_GPIO_PIN_BASE 0

// global variables
int nec_8_sm;
float clk_div = 1.0f;

//************************************************************************************************************

void nec_available_interrupt_handler()
{
    // display any frames received in the NEC FIFO
    if (pio_sm_is_rx_fifo_empty(pio0, nec_8_sm))
    {
        printf("?? NEC Rx FIFO is empty\n");
    }
    else
    {
        uint32_t rx_frame = pio_sm_get(pio0, nec_8_sm);

        uint8_t address, data, repeat;
        if (nec_decode_frame(rx_frame, &address, &data, &repeat))
        {
            // normally shouldn't do printf inside an interrupt handler...but it's not critical in this case, it's only for debug
            printf("received address= 0x%02x, data = 0x%02x, repeat = 0x%02x\n", address, data, repeat);
        }
        else
        {
            printf("received: 0x%08x\n", rx_frame);
        }
    }
}

//************************************************************************************************************

uint32_t getIntFromUser()
{
    // user could enter 000000 - 999999, add one more char for the null termination
    char buf[7];

    memset(buf, 0, sizeof(buf));

    // it stops reading when sizeof(buf)-1 chars are read
    fgets(buf, sizeof(buf), stdin);

    uint32_t number = atoi(buf);
    
    return number;
}

//************************************************************************************************************

int main() {
    stdio_init_all();
    
    // configure and enable the debug state machine
    nec_8_sm = nec_8_init(pio0, CAPTURE_GPIO_PIN_BASE);

    if (nec_8_sm == -1)
    {
        while (true)
        {
            printf("could not configure PIO\n");
            sleep_ms(1000);
        }
    }

    bool nec_interrupts_on = false;
    
    while (true)
    {
        int c = getchar_timeout_us(0);
        if ('i' == c)
        {
            // turn the interrupt on/off for automatic decoding of new NEC data
            nec_interrupts_on = !nec_interrupts_on;
            notify_new_nec_data(pio0, nec_8_sm, nec_available_interrupt_handler, nec_interrupts_on);
        }
        else if ('q' == c)
        {
            printf("checking NEC data running on sm %d\n", nec_8_sm);
            if (pio_sm_is_rx_fifo_empty(pio0, nec_8_sm))
            {
                printf("no NEC data available\n");
            }
            else
            {
                uint32_t rx_frame = pio_sm_get(pio0, nec_8_sm);
                printf("received: %08x\n", rx_frame);
            }
        }
        else if ('c' == c)
        {
            if (logic_analyzer_init(CAPTURE_GPIO_PIN_BASE, clk_div))
            {
                #define CAPTURE_BUFFER_WORDS 512
                uint32_t buffer[CAPTURE_BUFFER_WORDS];
                logic_analyzer_start(buffer, CAPTURE_BUFFER_WORDS, CAPTURE_GPIO_PIN_BASE);
                logic_analyzer_wait_for_complete();
                logic_analyzer_cleanup();

                for (int i = 0; i < CAPTURE_BUFFER_WORDS; i++)
                {
                    for (int j = 0; j < 32; j++)
                    {
                        bool bit = buffer[i] & (0x1 << j);
                        printf("%c", bit ? '1' : '0');
                    }
                    sleep_us(500);
                }
                printf("\n");
            }
            else
            {
                printf("could not initialize the logic analyzer\n");
            }
        }
        else if ('u' == c)
        {
            uint32_t us_per_sample = getIntFromUser();
            printf("will capture data once every %u microseconds\n", us_per_sample);
            clk_div = calc_clk_div_from_us(us_per_sample);
            printf("will use clk_div of %f\n", clk_div);
        }
        else if ('n' == c)
        {
            uint32_t ns_per_sample = getIntFromUser();
            printf("will capture data once every %u nanoseconds\n", ns_per_sample);
            clk_div = calc_clk_div_from_ns(ns_per_sample);
            printf("will use clk_div of %f\n", clk_div);
        }
    }
}
