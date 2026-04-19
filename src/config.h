#pragma once
#include <stdint.h>
#include <stdbool.h>

////////////////////////////////////
// Buffer size for all word/date string buffers
#define BUFFER_SIZE 44

////////////////////////////////////
// Watch Face Variants
#define TimeRenderOh         true
#define DateSeparatorLine    false
#define DateOutsideJustified true
#define DateFormatUS         true
////////////////////////////////////

#if DateFormatUS
#define DateFormat "%m.%d"
#else
#define DateFormat "%d.%m"
#endif

#if DateOutsideJustified
#define DateTruncTo2Char false
#else
#define DateTruncTo2Char true
#endif

// ---------------------------------------------------------------------------
// Layout configuration — populated at runtime by layout_config_init()
// ---------------------------------------------------------------------------

typedef struct {
    int16_t     screen_width;
    int16_t     line_height;      // height of each time-word text layer
    int16_t     line_spacing;     // vertical distance between time-word rows
    int16_t     line1_y;          // y of time row 1
    int16_t     line2_y;          // y of time row 2
    int16_t     line3_y;          // y of time row 3
    int16_t     weekday_x;
    int16_t     weekday_w;
    int16_t     weekday_y;
    bool        weekday_right;
    int16_t     date_x;
    int16_t     date_w;
    int16_t     date_y;
    bool        date_right;
    int16_t     sep_y;            // y of the separator line
    int16_t     sep_inset;        // horizontal inset for separator line
    uint32_t    anim_duration;    // slide animation duration in ms
    const char *time_bold_font;
    const char *time_light_font;
    const char *date_font;
} LayoutConfig;
