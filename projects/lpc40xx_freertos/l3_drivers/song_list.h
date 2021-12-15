// @file: song_list.h

#pragma once

#include <stddef.h> // size_t

typedef char song_memory_t[128];

void song_list__populate(void);
size_t song_list__get_item_count(void);
const char *song_list__get_name_for_item(size_t item_number);