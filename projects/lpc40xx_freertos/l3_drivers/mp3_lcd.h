#pragma once
#include "gpio.h"
#include "lpc40xx.h"
#include <stdbool.h>

#define LCD_CLEAR_BIT 0x01
#define LCD_HOME_BIT 0X02
#define LCD_ENTRY_MODE_BIT 0x04
#define LCD_DISPLAY_TOGGLE_BIT 0x08
#define LCD_CURSOR_BIT 0x10
#define LCD_FUNCTION_BIT 0x20

void lcd_init();
void write_command(uint8_t data);
void print_to_screen(const char *str, bool return_to_home_flag);
void print_to_screen_with_arrow(const char *str, bool return_to_home_flag);
void print_to_position(const char *str, uint8_t line_no, uint8_t pos);

void clear_screen(void);
void return_to_home_position(void);
void set_entry_mode(uint8_t ID, uint8_t shift);
void display_toggle(uint8_t D, uint8_t cursor, uint8_t blink);
void cursor_or_display_shift(uint8_t shift_or_cursor, uint8_t direction);
void function_set(uint8_t data_length, uint8_t number_of_display_lines_in_binary, uint8_t font);
