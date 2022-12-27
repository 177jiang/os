#ifndef __jyos_timer_h_
#define __jyos_timer_h_

#include <datastructs/jlist.h>
#include <stdint.h>

#define SYS_TIMER_FREQUENCY_HZ      2048

#define TIMER_MODE_PERIODIC   0x1

struct timer_context {
    struct timer *active_timers;
    uint32_t base_frequency;
    uint32_t running_frequency;
    uint32_t tick_interval;
};

struct timer {
    struct list_header link;
    uint32_t deadline;
    uint32_t counter;
    void* payload;
    void (*callback)(void*);
    uint8_t flags;
};


/**
 * @brief Initialize the system timer that runs at specified frequency
 * 
 * @param frequency The frequency that timer should run in Hz.
 */
void timer_init(uint32_t frequency);

int timer_run_second(uint32_t second, void (*callback)(void*), void* payload, uint8_t flags);
int timer_run_ms(uint32_t millisecond, void (*callback)(void*), void* payload, uint8_t flags);

int timer_run(uint32_t ticks, void (*callback)(void*), void* payload, uint8_t flags);

struct timer_context * timer_context();

#endif
