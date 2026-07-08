#include "timer.h"
#include "LAGraph.h"

static double start_time = 0;
static bool timer_initialized = false;

void timer_start(void) {
    start_time = LAGraph_WallClockTime();
    timer_initialized = true;
}

double timer_stop_ms(void) {
    if (!timer_initialized) return 0.0;
    return (LAGraph_WallClockTime() - start_time) * 1000.0;
}

double timer_stop_sec(void) {
    if (!timer_initialized) return 0.0;
    return LAGraph_WallClockTime() - start_time;
}
