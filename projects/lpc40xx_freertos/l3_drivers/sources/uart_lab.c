#include "uart_lab.h"
#include "FreeRTOS.h"
#include "task.h"

#include "lpc40xx.h"
#include "lpc_peripherals.h"
static QueueHandle_t your_uart_rx_queue;

static void your_receive_interrupt(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  LPC_UART_TypeDef *Uart3_base_address;
  Uart3_base_address = LPC_UART3_BASE;
  LPC_UART_TypeDef *Uart2_base_address;
  Uart2_base_address = LPC_UART3_BASE;
  LPC_UART_TypeDef *Uart_base_address;
  if (!(Uart2_base_address->IIR & 0x01)) {
    if (Uart2_base_address->IIR & (0x01 << 2)) {
      Uart_base_address = Uart2_base_address;
    }
  } else if (!(Uart3_base_address->IIR & 0x01)) {
    if (Uart3_base_address->IIR & (0x01 << 2)) {
      Uart_base_address = Uart3_base_address;
    }
  } else {
    fprintf(stderr, "No UART Interrupts");
    ;
  }
  if (!(Uart_base_address->LSR & 0x01)) {
    fprintf(stderr, "No Data");
  }
  // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read

  // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
  const char byte = Uart_base_address->RBR;
  xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
}

// Public function to enable UART interrupt
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  LPC_UART_TypeDef *Uart_base_address;
  if (uart_number == UART_2) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interrupt, "uart2Interrupt");
    Uart_base_address = LPC_UART2_BASE;
  }

  if (uart_number == UART_3) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interrupt, "uart3Interrupt");
    Uart_base_address = LPC_UART3_BASE;
  }
  // TODO: Enable UART receive interrupt by reading the LPC User manual
  // Hint: Read about the IER register
  Uart_base_address->IER |= 0x01; // Enable Recieve Data Available Interrupt
  your_uart_rx_queue = xQueueCreate(8, sizeof(char));
}

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  LPC_UART_TypeDef *Uart_base_address;
  if (uart == UART_2) {
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART2);
    Uart_base_address = LPC_UART2_BASE;
  }

  if (uart == UART_3) {
    lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__UART3);
    Uart_base_address = LPC_UART3_BASE;
  }
  Uart_base_address->LCR |= (1 << 7); // set divsor latch access bit
  const uint16_t divider_16_bit = peripheral_clock / (16 * baud_rate);
  Uart_base_address->DLM = (divider_16_bit >> 8) & 0xFF;
  Uart_base_address->DLL = (divider_16_bit >> 0) & 0xFF;
  Uart_base_address->FDR |= (1 << 4); // makes baud rate divider work
  Uart_base_address->LCR = 3;         // 8 bit transfer and resets divsor latch bit
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  LPC_UART_TypeDef *Uart_base_address;
  if (uart == UART_2) {
    Uart_base_address = LPC_UART2_BASE;
  }

  if (uart == UART_3) {
    Uart_base_address = LPC_UART3_BASE;
  }

  if (Uart_base_address->LSR & 0x01) { // if data ready bit is set
    *input_byte = (char)Uart_base_address->RBR;
    return true;
  } else {
    return false;
  }
}
// a) Check LSR for Receive Data Ready
// b) Copy data from RBR register to input_byte

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  LPC_UART_TypeDef *Uart_base_address;
  if (uart == UART_2) {
    Uart_base_address = LPC_UART2_BASE;
  }

  if (uart == UART_3) {
    Uart_base_address = LPC_UART3_BASE;
  }

  if (!(Uart_base_address->LSR & 0x01)) { // if data ready bit is not set (empty)
    Uart_base_address->THR = (uint8_t)output_byte;
    return true;
  } else {
    return false;
  }
}
// a) Check LSR for Transmit Hold Register Empty
// b) Copy output_byte to THR register
