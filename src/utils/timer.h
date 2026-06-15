#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <stdbool.h>

typedef clock_t timer_timestamp_t;

void timer_start(void);

double timer_stop_ms(void);

double timer_stop_sec(void);

long timer_get_resolution(void);

bool timer_is_available(void);

#endif /* TIMER_H */
