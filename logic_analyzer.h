float calc_clk_div_from_us(uint32_t us_per_sample);
float calc_clk_div_from_ns(uint32_t ns_per_sample);
bool logic_analyzer_init(uint pin_base, uint16_t num_pins, float clk_div);
void logic_analyzer_start(uint32_t* buffer, int capture_size_words, uint trigger_pin, bool trigger_logic_high);
void logic_analyzer_wait_for_complete();
void logic_analyzer_cleanup();
