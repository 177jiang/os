#ifndef __jyos_clock_h_
#define __jyos_clock_h_

#include <stdint.h>

typedef uint32_t time_t;

typedef struct {
    uint32_t year;      // use int32 as we need to store the 4-digit year
    uint8_t  month;
    uint8_t  day;
    uint8_t  weekday;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} datetime_t;

void clock_init();

void time_getdatetime(datetime_t* datetime);

int clock_datetime_eq(datetime_t* a, datetime_t *b);

time_t clock_systime();

#endif
