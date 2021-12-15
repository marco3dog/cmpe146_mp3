#include <stdio.h>

#include "FreeRTOS.h"
#include "adc.h"
#include "errno.h"
#include "event_groups.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "i2c.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "mp3_lcd.h"
#include "mp3_menu_system.h"
#include "pwm1.h"
#include "queue.h"
#include "semphr.h"
#include "song_list.h"
#include "ssp0_mp3.h"
#include "ssp2_lab.h"
#include "task.h"
#include "timers.h"
#include "uart_lab.h"
#include <string.h>

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"
#include "sys_time.h"

typedef char songname[32];

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

uint16_t volumes[8] = {(1991 * 7), (1991 * 6), (1991 * 5), (1991 * 4), (1991 * 3), (1991 * 2), 1991, 0x0000};
size_t volume_level;
uint64_t time_since_button_press;

SemaphoreHandle_t song_number_change;
SemaphoreHandle_t song_select;
SemaphoreHandle_t volume_change_semaphore;
SemaphoreHandle_t printing_to_screen_semaphore;
SemaphoreHandle_t change_song_semaphore;
SemaphoreHandle_t play_pause_semaphore;

gpio_s switch_down;
gpio_s switch_up;
gpio_s switch_select;
gpio_s switch_volume_up;
gpio_s switch_volume_down;

size_t song_number;

bool currently_playing;
bool now_playing_screen_flag;
bool stop_playback;

void mp3_player_task(void *p);
void mp3_reader_task(void *p);
void mp3_lcd_update_screen_task(void *p);
void print_now_playing_task(void *p);
void adjust_volume_task(void *p);

void scroll_down_isr(void);
void scroll_up_isr(void);
void select_isr(void);
void adjust_volume_up_isr(void);
void adjust_volume_down_isr(void);

int main(void) {
  sj2_cli__init();
  sys_time__init(clock__get_peripheral_clock_hz());

  now_playing_screen_flag = false;
  currently_playing = false;
  stop_playback = false;
  song_number = 0;
  volume_level = 3;
  time_since_button_press = 0;

  Q_songname = xQueueCreate(1, sizeof(songname));
  Q_songdata = xQueueCreate(1, 512);

  song_number_change = xSemaphoreCreateBinary();
  song_select = xSemaphoreCreateBinary();
  volume_change_semaphore = xSemaphoreCreateBinary();
  change_song_semaphore = xSemaphoreCreateBinary();
  printing_to_screen_semaphore = xSemaphoreCreateMutex();
  play_pause_semaphore = xSemaphoreCreateBinary();

  switch_down = gpio__construct_with_function(GPIO__PORT_0, 6, 0);
  switch_up = gpio__construct_with_function(GPIO__PORT_0, 7, 0);
  switch_select = gpio__construct_with_function(GPIO__PORT_0, 25, 0);
  switch_volume_up = gpio__construct_with_function(GPIO__PORT_0, 11, 0);
  switch_volume_down = gpio__construct_with_function(GPIO__PORT_0, 8, 0);

  gpio__set_as_input(switch_down);
  gpio__set_as_input(switch_up);
  gpio__set_as_input(switch_select);
  gpio__set_as_input(switch_volume_down);
  gpio__set_as_input(switch_volume_up);

  NVIC_EnableIRQ(GPIO_IRQn);
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "gpio_interrupt");
  gpio0__attach_interrupt(6, GPIO_INTR__RISING_EDGE, scroll_down_isr);
  gpio0__attach_interrupt(7, GPIO_INTR__RISING_EDGE, scroll_up_isr);
  gpio0__attach_interrupt(25, GPIO_INTR__FALLING_EDGE, select_isr); // 15
  gpio0__attach_interrupt(11, GPIO_INTR__FALLING_EDGE, adjust_volume_up_isr);
  gpio0__attach_interrupt(8, GPIO_INTR__FALLING_EDGE, adjust_volume_down_isr);

  ssp0_mp3_init();
  song_list__populate();
  main_menu_init();
  printf("Number of songs: %d\n", song_list__get_item_count());
  for (size_t i = 0; i < song_list__get_item_count(); i++) {
    printf("Name: %s\n", song_list__get_name_for_item(i));
  }
  printf("CLOCKF: %x\n", read_sci_reg(SCI_CLOCKF));
  printf("MODE: 0x%x\n", read_sci_reg(SCI_MODE));

  xTaskCreate(mp3_lcd_update_screen_task, "lcd_task", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_reader_task, "reader", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(mp3_player_task, "player", 4096 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(print_now_playing_task, "now_playing", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(adjust_volume_task, "adjust_volume", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

  vTaskStartScheduler();
  return 0;
}

void send_song_data_from_SD(songname name, char *bytes_512) {
  const char *filename = name;
  FIL file;
  UINT bytes_read = 0;
  FRESULT result = f_open(&file, filename, FA_READ);
  if (FR_OK == result) {
    FSIZE_t file_size = f_size(&file);
    char artist_name[512] = "";
    UINT bytes_read_2 = 0;
    f_lseek(&file, file_size - 512);
    f_read(&file, &artist_name[0], 512, &bytes_read_2);
    f_lseek(&file, 0);
    printf("Artist: %s\n", artist_name);
    printf("file size: %ld\n", file_size);
    currently_playing = true;
    while (!f_eof(&file)) {
      if (stop_playback) {
        f_close(&file);
        mp3_reset();
        write_sci_reg(SCI_MODE, SCI_MODE_INIT);
        write_sci_reg(SCI_CLOCKF, SCI_CLOCKF_INIT);
        break;
      }
      if (FR_OK == f_read(&file, &bytes_512[0], 512, &bytes_read)) {
        xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
      } else {
        printf("ERROR: Failed to read data to file\n");
      }
    }
    currently_playing = false;
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
    printf("Error: %d\n", result);
  }
}
void mp3_reader_task(void *p) {
  songname name;
  char bytes_512[512];

  while (1) {
    xQueueReceive(Q_songname, &name[0], portMAX_DELAY);
    vTaskDelay(5);
    write_sci_reg(SCI_MODE, SCI_MODE_INIT);
    send_song_data_from_SD(name, bytes_512);
  }
}

void mp3_player_task(void *p) {
  char bytes_512[512];
  uint16_t i = 0;
  while (1) {
    xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
    i = 0;
    while (i < sizeof(bytes_512)) {
      if (xSemaphoreTake(change_song_semaphore, 0)) {
        stop_playback = true;
        break;
      }
      while (!can_take_data()) {
        ;
      }
      xdcs_cs();
      ssp0_mp3_exchange_byte(bytes_512[i]);
      xdcs_ds();
      i++;
    }
  }
}

void mp3_lcd_update_screen_task(void *p) {
  while (1) {
    if (xSemaphoreTake(song_number_change, portMAX_DELAY)) {
      vTaskDelay(20);
      xSemaphoreTake(printing_to_screen_semaphore, portMAX_DELAY);
      now_playing_screen_flag = false;
      vTaskDelay(20);
      print_to_screen_with_arrow(song_list__get_name_for_item(song_number % song_list__get_item_count()),
                                 RESET_TO_HOME);
      print_to_screen(song_list__get_name_for_item((song_number + 1) % song_list__get_item_count()), START_NEW_LINE);
      print_to_screen(song_list__get_name_for_item((song_number + 2) % song_list__get_item_count()), START_NEW_LINE);
      print_to_screen(song_list__get_name_for_item((song_number + 3) % song_list__get_item_count()), START_NEW_LINE);
      xSemaphoreGive(printing_to_screen_semaphore);
    }
  }
}

void print_now_playing_task(void *p) {
  while (1) {
    xSemaphoreTake(song_select, portMAX_DELAY);
    vTaskDelay(500);
    xSemaphoreTake(printing_to_screen_semaphore, portMAX_DELAY);
    now_playing_screen_flag = true;
    clear_screen();
    print_to_screen("Now playing:", RESET_TO_HOME);
    print_to_screen(song_list__get_name_for_item(song_number), START_NEW_LINE);
    print_to_screen("Artist: asfdasdf", START_NEW_LINE); // PLACEHOLDER
    print_to_position("Vol:", 3, 0);
    xSemaphoreGive(printing_to_screen_semaphore);
    xSemaphoreGive(volume_change_semaphore);
  }
}

void print_volume_level_chars(void) {
  switch (volume_level) {
  case 0:
    print_to_position("Muted", 3, 4);
    break;
  case 1:
    print_to_position("=      ", 3, 4);
    break;
  case 2:
    print_to_position("==     ", 3, 4);
    break;
  case 3:
    print_to_position("===    ", 3, 4);
    break;
  case 4:
    print_to_position("====   ", 3, 4);
    break;
  case 5:
    print_to_position("=====  ", 3, 4);
    break;
  case 6:
    print_to_position("====== ", 3, 4);
    break;
  case 7:
    print_to_position("=======", 3, 4);
    break;
  default:;
  }
}

void adjust_volume_task(void *p) {
  while (1) {
    xSemaphoreTake(volume_change_semaphore, portMAX_DELAY);
    write_sci_reg(SCI_VOL, volumes[volume_level]);
    if (now_playing_screen_flag) {
      xSemaphoreTake(printing_to_screen_semaphore, portMAX_DELAY);
      print_volume_level_chars();
      xSemaphoreGive(printing_to_screen_semaphore);
    }
  }
}

void scroll_down_isr(void) {
  if (sys_time__get_uptime_ms() - time_since_button_press >= 250) {
    song_number++;
    if (song_number == song_list__get_item_count())
      song_number = 0;
    xSemaphoreGiveFromISR(song_number_change, NULL);
    time_since_button_press = sys_time__get_uptime_ms();
  }
}

void scroll_up_isr(void) {
  if (sys_time__get_uptime_ms() - time_since_button_press >= 250) {
    if (song_number != 0) {
      song_number--;
    } else {
      song_number = (song_list__get_item_count() - 1);
    }
    xSemaphoreGiveFromISR(song_number_change, NULL);
    time_since_button_press = sys_time__get_uptime_ms();
  }
}

void select_isr(void) {
  if (currently_playing && !now_playing_screen_flag) {
    xSemaphoreGiveFromISR(song_select, NULL);
    xSemaphoreGiveFromISR(change_song_semaphore, NULL);
  } else if (now_playing_screen_flag) {
    currently_playing = false;
    xSemaphoreGiveFromISR(play_pause_semaphore, NULL);

  } else if (!currently_playing) {
    xSemaphoreGiveFromISR(song_select, NULL);
  }

  xQueueSendFromISR(Q_songname, song_list__get_name_for_item(song_number % song_list__get_item_count()), 0);
}

void adjust_volume_up_isr(void) {
  if (sys_time__get_uptime_ms() - time_since_button_press >= 250) {
    if (volume_level >= 7) {
      ;
    } else {
      volume_level++;
      xSemaphoreGiveFromISR(volume_change_semaphore, NULL);
      time_since_button_press = sys_time__get_uptime_ms();
    }
  }
}

void adjust_volume_down_isr(void) {
  if (sys_time__get_uptime_ms() - time_since_button_press >= 250) {
    if (volume_level <= 0) {
      ;
    } else {
      volume_level--;
      xSemaphoreGiveFromISR(volume_change_semaphore, NULL);
      time_since_button_press = sys_time__get_uptime_ms();
    }
  }
}
