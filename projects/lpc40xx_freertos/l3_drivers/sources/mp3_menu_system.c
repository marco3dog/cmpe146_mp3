#include "mp3_menu_system.h"
#include "FreeRTOS.h"
#include "clock.h"
#include "delay.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "lpc_peripherals.h"
#include "mp3_lcd.h"
#include "song_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t number_of_songs;

extern size_t song_number;

void main_menu_init(void) {
  lcd_init();
  display_toggle(1, 0, 0);
  print_to_position("Welcome!", 1, 6);
  delay__ms(2000);
  clear_screen();
  print_to_screen("Please pick a song:", RESET_TO_HOME);
  delay__ms(2000);

  print_to_screen_with_arrow(song_list__get_name_for_item(song_number), RESET_TO_HOME);
  if (song_list__get_name_for_item(song_number + 1) != NULL)
    print_to_screen(song_list__get_name_for_item(song_number + 1), START_NEW_LINE);
  if (song_list__get_name_for_item(song_number + 2) != NULL)
    print_to_screen(song_list__get_name_for_item(song_number + 2), START_NEW_LINE);
  if (song_list__get_name_for_item(song_number + 3) != NULL)
    print_to_screen(song_list__get_name_for_item(song_number + 3), START_NEW_LINE);
}
