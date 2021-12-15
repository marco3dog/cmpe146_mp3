// @file gpio_isr.c
#include "gpio_isr.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_falling_callbacks[32];
static function_pointer_t gpio0_rising_callbacks[32];

static int pin_that_inted(void) {
  int data = 0;
  if (LPC_GPIOINT->IO0IntStatR != 0) {
    data = LPC_GPIOINT->IO0IntStatR;
  } else {
    data = LPC_GPIOINT->IO0IntStatF;
  }
  int counter = 0;
  while (data / 2 != 0) {
    data = data / 2;
    counter++;
  }
  return counter;
}

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  // 1) Store the callback based on the pin at gpio0_callbacks
  // 2) Configure GPIO 0 pin for rising or falling edge

  LPC_GPIO0->DIR &= ~(UINT32_C(1) << pin);
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    gpio0_falling_callbacks[pin] = callback;
    LPC_GPIOINT->IO0IntEnF |= (UINT32_C(1) << pin);
  } else {
    gpio0_rising_callbacks[pin] = callback;
    LPC_GPIOINT->IO0IntEnR |= (UINT32_C(1) << pin);
  }
}

// We wrote some of the implementation for you
void gpio0__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt
  const int pin_that_generated_interrupt = pin_that_inted();
  function_pointer_t attached_user_handler;
  if (LPC_GPIOINT->IO0IntStatR != 0) {
    attached_user_handler = gpio0_rising_callbacks[pin_that_generated_interrupt];
  } else {
    attached_user_handler = gpio0_falling_callbacks[pin_that_generated_interrupt];
  }

  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();
  LPC_GPIOINT->IO0IntClr |= (UINT32_C(1) << pin_that_generated_interrupt);
}