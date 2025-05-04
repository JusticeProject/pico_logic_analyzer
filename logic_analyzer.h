bool logic_analyzer_init(uint pin_base, uint32_t sample_period_us);
void logic_analyzer_start(uint32_t* buffer, int capture_size_words, uint trigger_pin);
void logic_analyzer_wait_for_complete();
void logic_analyzer_cleanup();
