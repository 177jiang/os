#include <arch/x86/interrupts.h>

#include <hal/apic.h>
#include <hal/rtc.h>
#include <hal/cpu.h>

#include <mm/kalloc.h>
#include <spike.h>
#include <clock.h>
#include <sched.h>
#include <timer.h>
#include <libc/stdio.h>

#include <stdint.h>

#define LVT_ENTRY_TIMER(vector, mode) (LVT_DELIVERY_FIXED | mode | vector)

static void temp_intr_routine_rtc_tick(const isr_param *param);
static void temp_intr_routine_apic_timer(const isr_param *param);
static void timer_update(const isr_param *param);

static volatile struct timer_context *timer_ctx;

static volatile uint32_t rtc_counter = 0;
static volatile uint8_t apic_timer_done = 0;

static volatile uint32_t sched_ticks = 0;
static volatile uint32_t sched_ticks_counter = 0;

#define APIC_CALIBRATION_CONST 0x100000

void timer_init_context() {
    timer_ctx =
      (struct timer_context*)kmalloc(sizeof(struct timer_context));

    assert_msg(timer_ctx, "Fail to initialize timer contex");

    timer_ctx->active_timers =
      (struct timer*)kmalloc(sizeof(struct timer));
    list_init_head((struct list_header*)timer_ctx->active_timers);
}

void timer_init(uint32_t frequency){

    timer_init_context();

    cpu_disable_interrupt();

    // Setup APIC timer
    // Setup a one-shot timer, we will use this to measure the bus speed. So we
    // can
    //   then calibrate apic timer to work at *nearly* accurate hz
    apic_write_reg(APIC_TIMER_LVT,
                   LVT_ENTRY_TIMER(APIC_TIMER_IV, LVT_TIMER_ONESHOT));

    // Set divider to 64
    apic_write_reg(APIC_TIMER_DCR, APIC_TIMER_DIV64);

    /*
        Timer calibration process - measure the APIC timer base frequency

         step 1: setup a temporary isr for RTC timer which trigger at each tick
                 (1024Hz) 
         step 2: setup a temporary isr for #APIC_TIMER_IV 
         step 3: setup the divider, APIC_TIMER_DCR 
         step 4: Startup RTC timer 
         step 5: Write a large value, v, to APIC_TIMER_ICR to start APIC timer (this must be
                 followed immediately after step 4) 
          step 6: issue a write to EOI and clean up.

        When the APIC ICR counting down to 0 #APIC_TIMER_IV triggered, save the
       rtc timer's counter, k, and disable RTC timer immediately (although the
       RTC interrupts should be blocked by local APIC as we are currently busy
       on handling #APIC_TIMER_IV)

        So the apic timer frequency F_apic in Hz can be calculate as
            v / F_apic = k / 1024
            =>  F_apic = v / k * 1024

    */

    timer_ctx->base_frequency = 0;
    rtc_counter = 0;
    apic_timer_done = 0;

    intr_setvector(APIC_TIMER_IV, temp_intr_routine_apic_timer);
    intr_setvector(RTC_TIMER_IV, temp_intr_routine_rtc_tick);

    rtc_enable_timer();                                     // start RTC timer
    apic_write_reg(APIC_TIMER_ICR, APIC_CALIBRATION_CONST); // start APIC timer

    // while(1);
    // enable interrupt, just for our RTC start ticking!
    cpu_enable_interrupt();

    wait_until(apic_timer_done);

    // cpu_disable_interrupt();

    assert_msg(timer_ctx->base_frequency, "Fail to initialize timer (NOFREQ)");

    printf_("Base frequency: %u Hz\n", timer_ctx->base_frequency);

    timer_ctx->running_frequency = frequency;
    timer_ctx->tick_interval = timer_ctx->base_frequency / frequency;

    // cleanup
    intr_unsetvector(APIC_TIMER_IV, temp_intr_routine_apic_timer);
    intr_unsetvector(RTC_TIMER_IV, temp_intr_routine_rtc_tick);

    apic_write_reg(APIC_TIMER_LVT,
                   LVT_ENTRY_TIMER(APIC_TIMER_IV, LVT_TIMER_PERIODIC));

    intr_setvector(APIC_TIMER_IV, timer_update);

    apic_write_reg(APIC_TIMER_ICR, timer_ctx->tick_interval);

    sched_ticks         = timer_ctx->running_frequency / 1000 * SCHED_TIME_SLICE;
    sched_ticks_counter = 0;

}

int timer_run_second(uint32_t second, void (*callback)(void*), void* payload, uint8_t flags) {
    return timer_run(second * timer_ctx->running_frequency, callback, payload, flags);
}

int timer_run_ms(uint32_t millisecond, void (*callback)(void*), void* payload, uint8_t flags) {
    return timer_run(timer_ctx->running_frequency / 1000 * millisecond, callback, payload, flags);
}

int timer_run(uint32_t ticks, void (*callback)(void*), void* payload, uint8_t flags) {
    struct timer *timer = (struct timer*)kmalloc(sizeof(struct timer));

    if (!timer) return 0;

    timer->callback = callback;
    timer->counter  = ticks;
    timer->deadline = ticks;
    timer->payload  = payload;
    timer->flags    = flags;

    list_append((struct list_header*)timer_ctx->active_timers, &timer->link);

    return 1;
}

static void timer_update(const isr_param* param) {

    struct timer *pos, *n;
    struct timer* timer_list_head = timer_ctx->active_timers;

    list_for_each(pos, n, &timer_list_head->link, link) {
        if (--pos->counter) {
            continue;
        }

        pos->callback ? pos->callback(pos->payload) : 1;

        if (pos->flags & TIMER_MODE_PERIODIC) {
            pos->counter = pos->deadline;
        } else {
            list_delete(&pos->link);
            kfree(pos);
        }
    }

    if(sched_ticks_counter++ >= sched_ticks){
        sched_ticks_counter = 0;
        schedule();
    }

}

static void temp_intr_routine_rtc_tick(const isr_param* param) {
    rtc_counter++;

    // dummy read on register C so RTC can send anther interrupt
    //  This strange behaviour observed in virtual box & bochs
    (void)rtc_read_reg(RTC_REG_C);
}

static void temp_intr_routine_apic_timer(const isr_param* param) {
    timer_ctx->base_frequency =
      APIC_CALIBRATION_CONST / rtc_counter * RTC_TIMER_BASE_FREQUENCY;
    apic_timer_done = 1;

    rtc_disable_timer();
}

struct timer_context *timer_context(){
    return timer_ctx;
}
