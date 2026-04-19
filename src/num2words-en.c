#include "num2words-en.h"
#include "string.h"
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
  "eightteen",
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

static size_t append_string(char *buffer, size_t remaining, const char *str) {
  if (remaining == 0) return 0;
  size_t len = strlen(str);
  strncat(buffer, str, remaining - 1);
  return (len < remaining) ? len : remaining - 1;
}

static size_t append_number(char *words, size_t remaining, int num, short oh) {
  int tens_val = num / 10 % 10;
  int ones_val = num % 10;
  size_t written = 0;

  if (tens_val == 1 && num != 10) {
    return append_string(words, remaining, TEENS[ones_val]);
  }
  if ((num != 0) && ((tens_val != 0) || oh)) {
    written += append_string(words, remaining - written, TENS[tens_val]);
    if (ones_val > 0) {
      written += append_string(words, remaining - written, " ");
    }
  }
  if (ones_val > 0 || num == 0) {
    written += append_string(words, remaining - written, ONES[ones_val]);
  }
  return written;
}

void time_to_words(int hours, int minutes, char *words, size_t length) {
  size_t remaining = length;
  memset(words, 0, length);

  if (hours == 0 || hours == 12) {
    remaining -= append_string(words, remaining, TEENS[2]);
  } else {
    remaining -= append_number(words, remaining, hours % 12, false);
  }

  remaining -= append_string(words, remaining, " ");
  remaining -= append_number(words, remaining, minutes, TimeRenderOh);
  remaining -= append_string(words, remaining, " ");
}

void time_to_3words(int hours, int minutes, char *line1, char *line2, char *line3, size_t length, bool split_teens) {
  char value[BUFFER_SIZE];
  time_to_words(hours, minutes, value, length);

  memset(line1, 0, length);
  memset(line2, 0, length);
  memset(line3, 0, length);

  char *start = value;
  char *pch = strstr(start, " ");
  while (pch != NULL) {
    size_t word_len = (size_t)(pch - start);
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
  if (split_teens && strstr(line2, "teen") != 0) {
    if (!((strstr(line2, "thir") != 0) ||
          (strstr(line2, "fif")  != 0) ||
          (strstr(line2, "six")  != 0))) {
      char *teen = strstr(line2, "teen");
      if (teen) {
        memcpy(line3, teen, 4);
        teen[0] = 0;
      }
    }
  }
}
