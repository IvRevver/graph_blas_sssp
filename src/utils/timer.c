#include "timer.h"
#include <stdio.h>

static timer_timestamp_t start_time = 0;

static bool timer_initialized = false;

void timer_start(void) {
    start_time = clock();
    timer_initialized = true;
}

double timer_stop_ms(void) {
    if (!timer_initialized) {
        return 0.0;
    }
    
    timer_timestamp_t end_time = clock();
    clock_t ticks = end_time - start_time;    
    double milliseconds = ((double)ticks / CLOCKS_PER_SEC) * 1000.0;
    return milliseconds;
}

double timer_stop_sec(void) {
    if (!timer_initialized) {
        return 0.0;
    }
    
    timer_timestamp_t end_time = clock();
    clock_t ticks = end_time - start_time;
    
    /* Конвертация в секунды */
    double seconds = (double)ticks / CLOCKS_PER_SEC;
    
    return seconds;
}

long timer_get_resolution(void) {
    return CLOCKS_PER_SEC;
}

bool timer_is_available(void) {
    return true;
}
