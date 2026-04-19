#include "pebble.h"
#include "settings.h"

#define KEY_TIME_RENDER_OH       0
#define KEY_DATE_SEPARATOR_LINE  1
#define KEY_DATE_OUTSIDE_JUST    2
#define KEY_DATE_FORMAT_US       3

void settings_load(Settings *s) {
    s->time_render_oh      = persist_exists(KEY_TIME_RENDER_OH)      ? persist_read_bool(KEY_TIME_RENDER_OH)      : true;
    s->date_separator_line = persist_exists(KEY_DATE_SEPARATOR_LINE) ? persist_read_bool(KEY_DATE_SEPARATOR_LINE) : false;
    s->date_outside_just   = persist_exists(KEY_DATE_OUTSIDE_JUST)   ? persist_read_bool(KEY_DATE_OUTSIDE_JUST)   : true;
    s->date_format_us      = persist_exists(KEY_DATE_FORMAT_US)      ? persist_read_bool(KEY_DATE_FORMAT_US)      : true;
}

void settings_save(const Settings *s) {
    persist_write_bool(KEY_TIME_RENDER_OH,      s->time_render_oh);
    persist_write_bool(KEY_DATE_SEPARATOR_LINE, s->date_separator_line);
    persist_write_bool(KEY_DATE_OUTSIDE_JUST,   s->date_outside_just);
    persist_write_bool(KEY_DATE_FORMAT_US,      s->date_format_us);
}
