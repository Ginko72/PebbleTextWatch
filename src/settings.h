#pragma once
#include <stdbool.h>

typedef struct {
    bool time_render_oh;
    bool date_separator_line;
    bool date_outside_just;
    bool date_format_us;
} Settings;

void settings_load(Settings *s);
void settings_save(const Settings *s);
