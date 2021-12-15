#include "i2c_slave_init.h"
#include "i2c.h"

void i2c2__slave_init(uint8_t slave_address_to_respond_to) {
  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  static StaticSemaphore_t binary_semaphore_struct;
  static StaticSemaphore_t mutex_struct;
  i2c__initialize(I2C__1, i2c_speed_hz, clock__get_peripheral_clock_hz(), &binary_semaphore_struct, &mutex_struct);
  LPC_I2C1->MASK0 = LPC_I2C1->MASK1 = LPC_I2C1->MASK2 = LPC_I2C1->MASK3 =
      0; // Masks should be 00 so it only responds 1 adr
  LPC_I2C1->ADR0 = LPC_I2C1->ADR1 = LPC_I2C1->ADR2 = LPC_I2C1->ADR3 = slave_address_to_respond_to; // only 1 slave
                                                                                                   // device
  LPC_I2C1->CONSET |= (1 << 2);
}