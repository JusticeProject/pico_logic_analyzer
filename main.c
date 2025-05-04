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
uint32_t sample_period_us = 1;

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
        if ('n' == c)
        {
            // turn the interrupt on/off for automatic decoding of new NEC data
            nec_interrupts_on = !nec_interrupts_on;
            notify_new_nec_data(pio0, nec_8_sm, nec_available_interrupt_handler, nec_interrupts_on);
        }
        /*else if ('q' == c)
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
        }*/
        else if ('c' == c)
        {
            if (logic_analyzer_init(CAPTURE_GPIO_PIN_BASE, sample_period_us))
            {
                #define CAPTURE_BUFFER_WORDS 512
                uint32_t buffer[CAPTURE_BUFFER_WORDS];
                logic_analyzer_start(buffer, CAPTURE_BUFFER_WORDS, CAPTURE_GPIO_PIN_BASE);
                logic_analyzer_wait_for_complete();
                logic_analyzer_cleanup();

                for (int i = 0; i < CAPTURE_BUFFER_WORDS; i++)
                {
                    printf("0x%08x\n", buffer[i]);
                }
            }
            else
            {
                printf("could not initialize the logic analyzer\n");
            }
        }
        else if ('t' == c)
        {
            sample_period_us = getIntFromUser();
            if (sample_period_us == 0)
            {
                sample_period_us = 1;
            }
            printf("will capture data once every %u microseconds\n", sample_period_us);
        }
    }
}
