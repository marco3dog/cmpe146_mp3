#pragma once

#include <stdint.h>

#include "function_types.h"

/**
 * The type of timers supported by LPC40xx
 * Note that for the SJ-2 project, lpc_timer1 is being used by to keep track of 'up time'
 */
typedef enum {
  lpc_timer0 = 0,
  lpc_timer1,
  lpc_timer2,
  lpc_timer3,
} lpc_timer_e;

typedef enum {
  lpc_timer__mr0 = 0,
  lpc_timer__mr1,
  lpc_timer__mr2,
  lpc_timer__mr3,
} lpc_timer__mr_e;

/**
 * Enables and starts the timer
 * @param prescalar_divider This divider is applied to the clock source into the timer
 *
 * @param isr_callback      The ISR callback for the timer, including all Match-Register interrupts
 * @note The isr_callback may be NULL if the timer will not be configured for any match interrupts
 */
void hw_timer__enable(lpc_timer_e timer, const uint32_t prescalar_divider, function_type__void isr_callback);

/**
 * When the HW timer counts up and matches the mr_value of type lpc_timer__mr_e then it will generate
 * an interrupt and invoke the callback registerd during hw_timer__enable()
 */
void hw_timer__enable_match_isr(lpc_timer_e timer, lpc_timer__mr_e mr_type, const uint32_t mr_value);

uint32_t hw_timer__get_value(lpc_timer_e timer);
