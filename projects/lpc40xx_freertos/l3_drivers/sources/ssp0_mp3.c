#include "ssp0_mp3.h"
#include "clock.h"
#include "gpio_isr.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stddef.h>

static const gpio_s xcs = {2, 5};    // SCI CS p2.0
static const gpio_s xdcs = {0, 1};   // SDI CS p2.1
static const gpio_s dreq = {0, 0};   // dreq
static const gpio_s reset = {0, 22}; // reset
static const gpio_s sck = {0, 15};
static const gpio_s miso = {0, 17};
static const gpio_s mosi = {0, 18};
void xcs_cs() { LPC_GPIO2->CLR |= (1 << 5); }

void xcs_ds() { LPC_GPIO2->SET |= (1 << 5); }

void xdcs_cs() { LPC_GPIO0->CLR |= (1 << 1); }

void xdcs_ds() { LPC_GPIO0->SET |= (1 << 1); }

bool can_take_data() { return gpio__get(dreq); }
void ssp0__initialize(uint32_t max_clock_khz);
void ssp0_mp3_init() {
  ssp0__initialize(1000);
  gpio__construct_with_function(2, 5, 0);  // SCI CS p2.0
  gpio__construct_with_function(0, 1, 0);  // SDI CS p2.1
  gpio__construct_with_function(0, 0, 0);  // dreq
  gpio__construct_with_function(0, 22, 0); // reset
  gpio__construct_with_function(0, 15, 2); // ssp0 sck
  gpio__construct_with_function(0, 17, 2); // MISO
  gpio__construct_with_function(0, 18, 2); // MOSI
  gpio__set_as_output(xcs);
  gpio__set_as_output(xdcs);
  gpio__set_as_input(dreq);
  gpio__set_as_output(reset);
  gpio__set_as_output(sck);
  gpio__set_as_input(miso);
  gpio__set_as_output(mosi);
  mp3_reset();
  xcs_ds();
  xdcs_ds();
  delay__ms(200);
  init_sci_regs();
}

void ssp0__set_max_clock(uint32_t max_clock_khz) {
  uint8_t divider = 2;
  const uint32_t cpu_clock_khz = clock__get_core_clock_hz() / 1000UL;

  // Keep scaling down divider until calculated is higher
  while (max_clock_khz < (cpu_clock_khz / divider) && divider <= 254) {
    divider += 2;
  }

  LPC_SSP0->CPSR = divider;
}

void ssp0__initialize(uint32_t max_clock_khz) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP0);

  LPC_SSP0->CR0 = 7;        // 8-bit mode
  LPC_SSP0->CR1 = (1 << 1); // Enable SSP as Master
  ssp0__set_max_clock(max_clock_khz);
}

uint8_t ssp0_mp3_exchange_byte(uint8_t data_out) {
  LPC_SSP0->DR = data_out;

  while (LPC_SSP0->SR & (1 << 4)) { // will keep looping until busy bit is 0 (bit 4 not busy)
    ;                               // Wait until SSP is busy
  }

  return (uint8_t)(LPC_SSP0->DR & 0xFF);
}

void write_sci_reg(uint8_t address, uint16_t data) {
  while (!can_take_data()) {
    ;
  }
  xcs_cs();
  ssp0_mp3_exchange_byte(WRITE);
  ssp0_mp3_exchange_byte(address);
  uint8_t upper = (data >> 8 & 0xFF);
  uint8_t lower = (data & 0xFF);
  ssp0_mp3_exchange_byte(upper);
  ssp0_mp3_exchange_byte(lower);
  while (!can_take_data())
    ;
  xcs_ds();
}

void init_sci_regs() {
  write_sci_reg(SCI_MODE, SCI_MODE_INIT);
  delay__ms(200);
  write_sci_reg(SCI_VOL, SCI_VOL_INIT);
  delay__ms(200);
  // write_sci_reg(SCI_AUDATA, SCI_AUDATA_INIT);
  write_sci_reg(SCI_CLOCKF, SCI_CLOCKF_INIT);
}

void mp3_reset() {
  LPC_GPIO0->CLR |= (1 << 22);
  delay__ms(200);
  LPC_GPIO0->SET |= (1 << 22);
}

uint16_t read_sci_reg(uint8_t address) {
  while (!can_take_data()) {
    ;
  }
  xcs_cs();
  ssp0_mp3_exchange_byte(READ);
  ssp0_mp3_exchange_byte(address);
  uint16_t upper = (ssp0_mp3_exchange_byte(0) & 0x00FF);
  uint8_t lower = ssp0_mp3_exchange_byte(0);
  upper = (upper << 8);
  upper |= (lower & 0xFF);
  xcs_ds();
  return upper;
}