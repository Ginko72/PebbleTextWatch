#pragma once
#include "string.h"
#include <stdbool.h>

void time_to_words(int hours, int minutes, char* words, size_t length);
void time_to_3words(int hours, int minutes, char *line1, char *line2, char *line3, size_t length, bool split_teens);
