#include "pico/stdlib.h"
#include "hardware/pio.h"

//************************************************************************************************************

int nec_8_init(PIO pio, uint pin);
bool nec_decode_frame(uint32_t sm, uint8_t* p_address, uint8_t* p_data);
