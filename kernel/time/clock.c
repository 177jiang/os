#include <clock.h>
#include <timer.h>
#include <hal/rtc.h>
#include <spike.h>

#include <stddef.h>

static volatile time_t sys_time;

void clock_systime_counter(void* arg) {
    sys_time++;
}

void clock_init() {

    if (!timer_context()) {
        assert_msg(0,("Systimer not initialized"));
    }
    
    // 系统计时器每毫秒累加。
    timer_run_ms(1, clock_systime_counter, NULL, TIMER_MODE_PERIODIC);
}


int clock_datatime_eq(datetime_t* a, datetime_t* b) {
    return a->year == b->year &&
           a->month == b->month &&
           a->day == b->day &&
           a->weekday == b->weekday &&
           a->minute == b->minute &&
           a->second == b->second;
}

void time_getdatetime(datetime_t* datetime) {
    datetime_t current;
    
    do
    {
        while (rtc_read_reg(RTC_REG_A) & 0x80);
        memcpy(&current, datetime, sizeof(datetime_t));

        datetime->year = rtc_read_reg(RTC_REG_YRS);
        datetime->month = rtc_read_reg(RTC_REG_MTH);
        datetime->day = rtc_read_reg(RTC_REG_DAY);
        datetime->weekday = rtc_read_reg(RTC_REG_WDY);
        datetime->hour = rtc_read_reg(RTC_REG_HRS);
        datetime->minute = rtc_read_reg(RTC_REG_MIN);
        datetime->second = rtc_read_reg(RTC_REG_SEC);
    } while (!clock_datatime_eq(datetime, &current));

    uint8_t regbv = rtc_read_reg(RTC_REG_B);

    // Convert from bcd to binary when needed
    if (!RTC_BIN_ENCODED(regbv)) {
        datetime->year = bcd2dec(datetime->year);
        datetime->month = bcd2dec(datetime->month);
        datetime->day = bcd2dec(datetime->day);
        datetime->hour = bcd2dec(datetime->hour);
        datetime->minute = bcd2dec(datetime->minute);
        datetime->second = bcd2dec(datetime->second);
    }


    // To 24 hour format
    if (!RTC_24HRS_ENCODED(regbv) && (datetime->hour >> 7)) {
        datetime->hour = (12 + datetime->hour & 0x80);
    }

    datetime->year += RTC_CURRENT_CENTRY * 100;
}

void clock_walltime(datetime_t *dt){
    time_getdatetime(dt);
}
time_t clock_unixtime(){

    datetime_t dt;
    clock_walltime(&dt);
    return clock_tounixtime(&dt);
}

time_t clock_systime() {
    return sys_time;
}
