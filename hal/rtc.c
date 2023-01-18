#include <hal/rtc.h>
#include <clock.h>
#include <libc/string.h>
#include <libc/stdio.h>

void rtc_init() {
    uint8_t regA = rtc_read_reg(RTC_REG_A | WITH_NMI_DISABLED);
    regA = (regA & ~0x7f) | RTC_FREQUENCY_1024HZ | RTC_DIVIDER_33KHZ;
    rtc_write_reg(RTC_REG_A | WITH_NMI_DISABLED, regA);

    // Make sure the rtc timer is disabled by default
    rtc_disable_timer();
}

uint8_t rtc_read_reg(uint8_t reg_selector) {
    io_outb(RTC_INDEX_PORT, reg_selector);
    return io_inb(RTC_TARGET_PORT);
}

void rtc_write_reg(uint8_t reg_selector, uint8_t val) {
    io_outb(RTC_INDEX_PORT, reg_selector);
    io_outb(RTC_TARGET_PORT, val);
}

uint8_t bcd2dec(uint8_t bcd) {
    return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0xf);
}

int rtc_date_same(datetime_t* a, datetime_t* b) {
    return a->year == b->year &&
           a->month == b->month &&
           a->day == b->day &&
           a->weekday == b->weekday &&
           a->minute == b->minute &&
           a->second == b->second;
}


void rtc_enable_timer() {
    uint8_t regB = rtc_read_reg(RTC_REG_B | WITH_NMI_DISABLED);
    rtc_write_reg(RTC_REG_B | WITH_NMI_DISABLED, regB | RTC_TIMER_ON);
}

void rtc_disable_timer() {
    uint8_t regB = rtc_read_reg(RTC_REG_B | WITH_NMI_DISABLED);
    rtc_write_reg(RTC_REG_B | WITH_NMI_DISABLED, regB & ~RTC_TIMER_ON);
}
