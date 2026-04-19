#include "num2words-en.h"
#include <string.h>
#include <stdio.h>
#include "config.h"

static const char* const ONES[] = {
  "o'clock",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine"
};

static const char* const TEENS[] = {
  "",
  "eleven",
  "twelve",
  "thirteen",
  "fourteen",
  "fifteen",
  "sixteen",
  "seventeen",
  "eighteen",
  "nineteen"
};

static const char* const TENS[] = {
  "oh",
  "ten",
  "twenty",
  "thirty",
  "forty",
  "fifty",
  "sixty",
  "seventy",
  "eighty",
  "ninety"
};

static size_t append_string(char *buffer, size_t length, size_t pos, const char *str) {
  if (pos >= length - 1) return pos;
  int written = snprintf(buffer + pos, length - pos, "%s", str);
  return (written >= 0) ? pos + written : pos;
}

static size_t append_number(char *words, size_t length, size_t pos, int num, short oh) {
  int tens_val = num / 10 % 10;
  int ones_val = num % 10;
  size_t p = pos;
  
  if (tens_val == 1 && num != 10) {
    return append_string(words, length, p, TEENS[ones_val]);
  }
  if ((num != 0) && ((tens_val != 0) || oh)) {
    p = append_string(words, length, p, TENS[tens_val]);
    if (ones_val > 0) {
      p = append_string(words, length, p, " ");
    }
  }
  if (ones_val > 0 || num == 0) {
    p = append_string(words, length, p, ONES[ones_val]);
  }
  return p;
}

void time_to_words(int hours, int minutes, char *words, size_t length, bool oh) {
  size_t pos = 0;

  if (hours == 0 || hours == 12) {
    pos = append_string(words, length, pos, TEENS[2]);
  } else {
    pos = append_number(words, length, pos, hours % 12, false);
  }

  pos = append_string(words, length, pos, " ");
  pos = append_number(words, length, pos, minutes, oh);
  pos = append_string(words, length, pos, " ");
}

void time_to_3words(int hours, int minutes, char *line1, char *line2, char *line3, size_t length, bool split_teens, bool oh) {
  static char value[BUFFER_SIZE];
  time_to_words(hours, minutes, value, length, oh);

  memset(line1, 0, length);
  memset(line2, 0, length);
  memset(line3, 0, length);

  char *start = value;
  char *pch = strstr(start, " ");
  while (pch != NULL) {
    size_t word_len = (size_t)(pch - start);
    if (word_len >= length) word_len = length - 1;
    if (line1[0] == 0) {
      memcpy(line1, start, word_len);
    } else if (line2[0] == 0) {
      memcpy(line2, start, word_len);
    } else if (line3[0] == 0) {
      memcpy(line3, start, word_len);
    }
    start = pch + 1;
    pch = strstr(start, " ");
  }

  // On narrow screens, split long teen words (e.g. "seventeen" → "seven"/"teen")
  char *teen = strstr(line2, "teen");
  if (split_teens && teen != NULL) {
    if (!((strstr(line2, "thir") != 0) ||
          (strstr(line2, "fif")  != 0) ||
          (strstr(line2, "six")  != 0))) {
      size_t copy_len = length > 4 ? 4 : length - 1;
      if (strcmp(line2, "eighteen") == 0) {
        // Special case: split "eighteen" into "eight" and "teen"
        memcpy(line3, teen, copy_len);
        line2[5] = 0;
      } else {
        memcpy(line3, teen, copy_len);
        teen[0] = 0;
      }
    }
  }
}
