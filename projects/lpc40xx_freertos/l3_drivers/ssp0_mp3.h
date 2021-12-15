#pragma once

#include "FreeRTOS.h"
#include "delay.h"
#include "gpio.h"
#include "queue.h"
#include "semphr.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// 1053 SCI Registers
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_VOL 0x0B

// Command Types
#define READ 0x03
#define WRITE 0x02
// Init Values
#define SCI_MODE_INIT 0x4800
#define SCI_CLOCKF_INIT 0x6000
#define SCI_VOL_INIT 0x0101
#define SCI_AUDATA_INIT 0xAC45 // 44100 hz
#define SCI_BASS_INIT 0x0000
#define SCI_WRAM_INIT 0x1E06
// Pause
#define SCI_MODE_PAUSE 0x4808

void ssp0_mp3_init(); // 1 MHZ

uint8_t ssp0_mp3_exchange_byte(uint8_t data_out);

void write_sci_reg(uint8_t address, uint16_t data);

void init_sci_regs();

void mp3_reset();

bool can_take_data();

void xcs_cs();

void xcs_ds();

void xdcs_cs();

void xdcs_ds();

uint16_t read_sci_reg(uint8_t address);