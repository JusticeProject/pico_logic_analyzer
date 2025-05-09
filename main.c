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

uint32_t getLongIntFromUser()
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

uint16_t getShortIntFromUser()
{
    // user could enter 00 - 99, add one more char for the null termination
    char buf[3];

    memset(buf, 0, sizeof(buf));

    // it stops reading when sizeof(buf)-1 chars are read
    fgets(buf, sizeof(buf), stdin);

    uint16_t number = atoi(buf);
    
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
    bool trigger_logic_high = false;
    uint16_t num_pins = 1;
    
    while (true)
    {
        int c = getchar_timeout_us(0);
        if ('h' == c)
        {
            // TODO: update when new commands are added
            printf("\n");
            printf("i = toggle interrupt notification of new NEC data (default is off)\n");
            printf("q = query for new NEC data\n");
            printf("c = start capture\n");
            printf("u = set sample period in us, ex: u000056\n");
            printf("n = set sample period in ns, ex: n000100 (default is 8ns)\n");
            printf("p = set number of pins to capture (default is 1)\n");
            printf("o = start capture when logic 1 is detected\n");
            printf("z = start capture when logic 0 is detected (default)\n");
        }
        else if ('i' == c)
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
            if (logic_analyzer_init(CAPTURE_GPIO_PIN_BASE, num_pins, clk_div))
            {
                // TODO: if RASPBERRYPI_PICO_W or  RASPBERRYPI_PICO defined then smaller buffer
                //  else if RASPBERRYPI_PICO2_W or RASPBERRYPI_PICO2 defined then larger buffer
                // else unsupported platform
                // TODO: need to allow user configurable number of 32-bit words for capture, check that it's not larger than MAX_BUFFER_WORDS
                #define CAPTURE_BUFFER_WORDS 512
                

                uint32_t buffer[CAPTURE_BUFFER_WORDS];
                logic_analyzer_start(buffer, CAPTURE_BUFFER_WORDS, CAPTURE_GPIO_PIN_BASE, trigger_logic_high);
                logic_analyzer_wait_for_complete();
                logic_analyzer_cleanup();

                int chars_sent = 0;
                for (int pin = 0; pin < num_pins; pin++)
                {
                    for (int word = 0; word < CAPTURE_BUFFER_WORDS; word++)
                    {
                        for (int bit_location = pin; bit_location < 32; bit_location += num_pins)
                        {
                            bool data = buffer[word] & (0x1 << bit_location);
                            printf("%c", data ? '1' : '0');
                            chars_sent++;
                            if (chars_sent % 32 == 0)
                            {
                                sleep_us(100);
                            }
                        }
                        
                    }
                    printf("\n");
                }
            }
            else
            {
                printf("could not initialize the logic analyzer\n");
            }
        }
        else if ('u' == c)
        {
            uint32_t us_per_sample = getLongIntFromUser();
            printf("will capture data once every %u microseconds\n", us_per_sample);
            clk_div = calc_clk_div_from_us(us_per_sample);
            printf("will use clk_div of %f\n", clk_div);
        }
        else if ('n' == c)
        {
            uint32_t ns_per_sample = getLongIntFromUser();
            printf("will capture data once every %u nanoseconds\n", ns_per_sample);
            clk_div = calc_clk_div_from_ns(ns_per_sample);
            printf("will use clk_div of %f\n", clk_div);
        }
        else if ('p' == c)
        {
            num_pins = getShortIntFromUser();
            // We are enforcing a power of 2 for the number of pins. Could maybe use the log2 function 
            // but I don't want to deal with floating point rounding error.
            if ((1 == num_pins) || (2 == num_pins) || (4 == num_pins) || (8 == num_pins) || (16 == num_pins) || (32 == num_pins))
            {
                printf("will capture data on %u pins\n", num_pins);
            }
            else
            {
                printf("number of pins must be a power of 2 (1, 2, 4, 8, 16, 32), reverting to default of 1\n");
                num_pins = 1;
            }
        }
        else if ('o' == c)
        {
            // we want trigger on logic 1
            trigger_logic_high = true;
            printf("will start capture on logic 1\n");
        }
        else if ('z' == c)
        {
            // we want trigger on logic 0
            trigger_logic_high = false;
            printf("will start capture on logic 0\n");
        }
    }
}
