#include "mp3_lcd.h"
#include "FreeRTOS.h"
#include "clock.h"
#include "delay.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "song_list.h"
#include <stdio.h>
#include <stdlib.h>

gpio_s data_arr[8];

// initialize data lines
gpio_s pin_D0;
gpio_s pin_D1;
gpio_s pin_D2;
gpio_s pin_D3;
gpio_s pin_D4;
gpio_s pin_D5;
gpio_s pin_D6;
gpio_s pin_D7;

// initialize control lines
gpio_s pin_RS;
gpio_s pin_RW;
gpio_s pin_E;

static uint8_t position = 0;
static uint8_t line = 0;

void gpio_pin_config_for_lcd(void) {
  pin_D0 = gpio__construct_with_function(GPIO__PORT_1, 20, 0); // D0
  data_arr[0] = pin_D0;
  gpio__set_as_output(pin_D0);
  gpio__reset(pin_D0);

  pin_D1 = gpio__construct_with_function(GPIO__PORT_2, 2, 0); // D1           //PROBLEM PART used to be port 1 pin 1
  data_arr[1] = pin_D1;
  gpio__set_as_output(pin_D1);
  gpio__reset(pin_D1);

  pin_D2 = gpio__construct_with_function(GPIO__PORT_1, 28, 0); // D2
  data_arr[2] = pin_D2;
  gpio__set_as_output(pin_D2);
  gpio__reset(pin_D2);

  pin_D3 = gpio__construct_with_function(GPIO__PORT_4, 28, 0); // D3
  data_arr[3] = pin_D3;
  gpio__set_as_output(pin_D3);
  gpio__reset(pin_D3);

  pin_D4 = gpio__construct_with_function(GPIO__PORT_2, 0, 0); // D4
  data_arr[4] = pin_D4;
  gpio__set_as_output(pin_D4);
  gpio__reset(pin_D4);

  pin_D5 = gpio__construct_with_function(GPIO__PORT_2, 7, 0); // D5
  data_arr[5] = pin_D5;
  gpio__set_as_output(pin_D5);
  gpio__reset(pin_D5);

  pin_D6 = gpio__construct_with_function(GPIO__PORT_0, 26, 0); // D6
  data_arr[6] = pin_D6;
  gpio__set_as_output(pin_D6);
  gpio__reset(pin_D6);

  pin_D7 = gpio__construct_with_function(GPIO__PORT_1, 31, 0); // D7
  data_arr[7] = pin_D7;
  gpio__set_as_output(pin_D7);
  gpio__reset(pin_D7);

  pin_RS = gpio__construct_with_function(GPIO__PORT_1, 14, 0); // RS
  gpio__set_as_output(pin_RS);
  pin_RW = gpio__construct_with_function(GPIO__PORT_4, 29, 0); // R/W
  gpio__set_as_output(pin_RW);
  pin_E = gpio__construct_with_function(GPIO__PORT_0, 9, 0); // E
  gpio__set_as_output(pin_E);
}

void lcd_init(void) {

  gpio_pin_config_for_lcd();
  gpio__reset(pin_RS);
  gpio__reset(pin_RW);
  gpio__set(pin_E);
  delay__ms(50);
  write_command(0x30);
  write_command(0x30);
  write_command(0x30);

  write_command(0x3C);
  write_command(0x08);
  write_command(0x01);
  write_command(0x06);
  write_command(0x0F);
  printf("LCD initialized...\n\n\n");
}

void wait_until_not_busy() {
  // printf("Waiting until data is not being processed...\n");
  gpio__set_as_input(pin_D7);

  // RS = 0, RW = 1
  gpio__reset(pin_RS);
  gpio__set(pin_RW);
  gpio__set(pin_E);
  delay__ms(10);
  while (gpio__get(pin_D7)) {
    ;
  }
  gpio__reset(pin_E);
}

void send_data(uint8_t data) {
  // printf("%c", (char)data);
  // printf("0x%x", data);
  // printf("\n");
  for (int i = 0; i < 8; i++) {
    gpio__set_as_output(data_arr[i]);
    if (data & 0x01) {
      gpio__set(data_arr[i]);
    } else {
      gpio__reset(data_arr[i]);
    }
    data >>= 1;
  }
  // while (1) {
  //   ;
  // }
  gpio__set(pin_E);
  delay__ms(1);
  gpio__reset(pin_E);
  delay__ms(1);
}

void write_command(uint8_t data) {
  wait_until_not_busy();
  // printf("Done with waiting...\n");

  // RS = 0, RW = 0
  gpio__reset(pin_RS);
  gpio__reset(pin_RW);
  send_data(data);
  delay__ms(10);
}

void set_position(uint8_t pos, uint8_t line_num) {
  if (line_num == 1) {
    pos += 0x40;
  } else if (line_num == 2) {
    pos += 0x14;
  } else if (line_num == 3) {
    pos += 0x54;
  }
  write_command(0x80 | pos);
}

void start_new_line(void) {
  if (line == 3) {
    set_position(0, 0);
    line = 0;
  } else {
    line++;
    set_position(0, line);
  }
  position = 0;
}

void print_char(uint8_t data) {
  wait_until_not_busy();

  if (position > 19)
    start_new_line();

  // RS = 1, RW = 0
  gpio__set(pin_RS);
  gpio__reset(pin_RW);
  send_data(data);
  delay__ms(10);

  position++;
}

void clear_screen(void) { write_command(LCD_CLEAR_BIT); }

void return_to_home_position(void) {
  clear_screen();
  write_command(LCD_HOME_BIT);
  position = 0;
  line = 0;
}

void set_entry_mode(uint8_t ID, uint8_t shift) {
  ID <<= 1;
  write_command((LCD_HOME_BIT | ID | shift));
}

void display_toggle(uint8_t D, uint8_t cursor, uint8_t blink) {
  D <<= 2;
  cursor <<= 1;
  // printf("Command %x being sent...\n", (LCD_DISPLAY_TOGGLE_BIT | D | cursor | blink));

  write_command((LCD_DISPLAY_TOGGLE_BIT | D | cursor | blink));
}

void cursor_or_display_shift(uint8_t shift_or_cursor, uint8_t direction) {
  shift_or_cursor <<= 3;
  direction <<= 2;
  // printf("Command %x being sent...\n", (LCD_CURSOR_BIT | shift_or_cursor | direction));
  write_command((LCD_CURSOR_BIT | shift_or_cursor | direction));
}

void function_set(uint8_t data_length, uint8_t number_of_display_lines_in_binary, uint8_t font) {
  data_length <<= 4;
  number_of_display_lines_in_binary <<= 3;
  font <<= 2;
  printf("%x", (LCD_FUNCTION_BIT | data_length | number_of_display_lines_in_binary | font));
  write_command((LCD_FUNCTION_BIT | data_length | number_of_display_lines_in_binary | font));
}

void string_print_loop(const char *str) {
  int i = 0;
  while (str[i] != '\0') {
    if (str[i] == '.' && str[i + 1] == 'm')
      break;
    print_char((uint8_t)str[i]);
    // printf("%c\n", str[i]);
    i++;
  }
}

void print_to_screen(const char *str, bool return_to_home_flag) {
  // printf("Line no: %d\n", line);
  if (return_to_home_flag)
    return_to_home_position();
  else {
    // printf("Starting newline\n");
    start_new_line();
  }

  string_print_loop(str);
}

void print_to_screen_with_arrow(const char *str, bool return_to_home_flag) {
  // printf("Line no: %d\n", line);
  if (return_to_home_flag)
    return_to_home_position();
  else {
    // printf("Starting newline\n");
    start_new_line();
  }

  print_char(0x7E);
  string_print_loop(str);
}

void print_to_position(const char *str, uint8_t line_no, uint8_t pos) {
  if (line_no > 3)
    return;
  position = pos;
  set_position(pos, line_no);

  string_print_loop(str);
}
