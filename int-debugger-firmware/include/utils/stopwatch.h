#ifndef INCLUDE_STOPWATCH_H
#define INCLUDE_STOPWATCH_H

#include <stdbool.h>

#include "target.h"
extern uint64_t stopwatch_time_ms;
extern uint32_t largest_time;
extern uint32_t smallest_time;
extern uint32_t cycles;
extern float timerIntervalNs;

void init_stopwatch(void);
void start_stopwatch(void);
uint16_t end_stopwatch(void);
void halt_stopwatch(void);
void resume_stopwatch(void);

#endif /* INCLUDE_STOPWATCH_H */
