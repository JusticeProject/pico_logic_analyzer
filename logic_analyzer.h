#include "hardware/pio.h"

void logic_analyzer_init(PIO pio, int* p_sm, int* p_offset, uint pin_num);
void logic_analyzer_restart(PIO pio, int sm, int offset);
