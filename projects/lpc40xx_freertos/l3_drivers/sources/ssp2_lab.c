#include "ssp2_lab.h"
#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stddef.h>

void ssp2_lab_init(uint32_t max_clock_mhz) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);
  LPC_SSP2->CR0 = 7;        // set bits 3:0 to 0111 for 8 bit transfer
  LPC_SSP2->CR1 = (1 << 1); // enable SSP on bus
  uint8_t divider = 2;
  const uint32_t cpu_clock_khz = clock__get_core_clock_hz() / 1000UL; // get cpu clock speed khz

  while ((max_clock_mhz / 1000UL) < (cpu_clock_khz / divider) && divider <= 254) { // needed for users to set max speed
    divider += 2;
  }

  LPC_SSP2->CPSR = divider; // always even number, makes ssp_clock = cpu_clock / divider, max is cpu_clock / 2
}

uint8_t ssp2_lab_exchange_byte(uint8_t data_out) {
  LPC_SSP2->DR = data_out;

  while (LPC_SSP2->SR & (1 << 4)) { // will keep looping until busy bit is 0 (bit 4 not busy)
    ;                               // Wait until SSP is busy
  }

  return (uint8_t)(LPC_SSP2->DR & 0xFF);
}