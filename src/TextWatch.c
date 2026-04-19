#include "pebble.h"

#include "num2words-en.h"
#include "config.h"

#ifndef DEBUG
#define DEBUG false
#endif

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

static void layout_config_init(LayoutConfig *cfg, GRect bounds) {
    cfg->screen_width  = bounds.size.w;
    cfg->anim_duration = 400;
    cfg->sep_inset     = 10;

    // Scale font, line metrics, and bottom anchors by screen width
    if (bounds.size.w >= 200) {
        // emery: 200 × 228
        cfg->line_height     = 60;
        cfg->line_spacing    = 51;
        cfg->weekday_y       = bounds.size.h - 55;
        cfg->sep_y           = bounds.size.h - 50;
        cfg->time_bold_font  = FONT_KEY_BITHAM_42_BOLD;
        cfg->time_light_font = FONT_KEY_BITHAM_42_LIGHT;
        cfg->date_font       = FONT_KEY_BITHAM_34_MEDIUM_NUMBERS;
    } else {
        // aplite, basalt, diorite, flint: 144 × 168
        cfg->line_height     = 50;
        cfg->line_spacing    = 37;
        cfg->weekday_y       = bounds.size.h - 45;
        cfg->sep_y           = bounds.size.h - 40;
        cfg->time_bold_font  = FONT_KEY_BITHAM_42_BOLD;
        cfg->time_light_font = FONT_KEY_BITHAM_42_LIGHT;
        cfg->date_font       = FONT_KEY_BITHAM_34_MEDIUM_NUMBERS;
    }

    cfg->line1_y = 0;
    cfg->line2_y = cfg->line_spacing;
    cfg->line3_y = 2 * cfg->line_spacing;
    cfg->date_y  = cfg->weekday_y + 8;

    // Scale the weekday/date column split proportionally to screen width
    int16_t w = bounds.size.w;
#if DateOutsideJustified
    // Weekday left-justified, date right-justified
    cfg->weekday_x     = 0;
    cfg->weekday_w     = w / 2;
    cfg->weekday_right = false;
    cfg->date_x        = w * 44 / 100;   // slight overlap mirrors original 64/144
    cfg->date_w        = w - cfg->date_x;
    cfg->date_right    = true;
#else
    // Weekday right-justified, date left-justified
    cfg->weekday_x     = 0;
    cfg->weekday_w     = w * 42 / 100;   // mirrors original 61/144
    cfg->weekday_right = true;
    cfg->date_x        = w * 45 / 100;   // mirrors original 65/144
    cfg->date_w        = w - cfg->date_x;
    cfg->date_right    = false;
#endif
}

static LayoutConfig layout;

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static Window *window;

typedef struct {
    TextLayer *currentLayer;
    TextLayer *nextLayer;
    PropertyAnimation *currentAnimation;
    PropertyAnimation *nextAnimation;
} Line;

static Line line1;
static Line line2;
static Line line3;
static Line line4;
static Line line5;

#if DateSeparatorLine
static Layer *lineDrawLayer;
#endif

#if DEBUG
static struct tm debug_time;
#endif

static char line1Str[2][BUFFER_SIZE];
static char line2Str[2][BUFFER_SIZE];
static char line3Str[2][BUFFER_SIZE];
static char line4Str[2][BUFFER_SIZE];
static char line5Str[2][BUFFER_SIZE];

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

// Reset the finished (now off-screen) layer back to its parked position
void animationStoppedHandler(Animation *animation, bool finished, void *context) {
    TextLayer *current = (TextLayer *)context;
    GRect rect = layer_get_frame(text_layer_get_layer(current));
    rect.origin.x = layout.screen_width;
    layer_set_frame(text_layer_get_layer(current), rect);
}

// Slide current out to the left and next in from the right
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next) {
    GRect to_next = layer_get_frame(text_layer_get_layer(next));
    to_next.origin.x -= layout.screen_width;

    line->nextAnimation = property_animation_create_layer_frame(
        text_layer_get_layer(next), NULL, &to_next);
    animation_set_duration((Animation *)line->nextAnimation, layout.anim_duration);
    animation_set_curve((Animation *)line->nextAnimation, AnimationCurveEaseOut);
    animation_schedule((Animation *)line->nextAnimation);

    GRect to_current = layer_get_frame(text_layer_get_layer(current));
    to_current.origin.x -= layout.screen_width;

    line->currentAnimation = property_animation_create_layer_frame(
        text_layer_get_layer(current), NULL, &to_current);
    animation_set_duration((Animation *)line->currentAnimation, layout.anim_duration);
    animation_set_curve((Animation *)line->currentAnimation, AnimationCurveEaseOut);
    animation_set_handlers((Animation *)line->currentAnimation, (AnimationHandlers) {
        .stopped = animationStoppedHandler
    }, current);
    animation_schedule((Animation *)line->currentAnimation);
}

// Update a line with a sliding animation to the new value
void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value) {
    GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
    TextLayer *current = (rect.origin.x == 0) ? line->currentLayer : line->nextLayer;
    TextLayer *next    = (current == line->currentLayer) ? line->nextLayer : line->currentLayer;

    if (current == line->currentLayer) {
        memset(lineStr[1], 0, BUFFER_SIZE);
        memcpy(lineStr[1], value, strlen(value));
        text_layer_set_text(next, lineStr[1]);
    } else {
        memset(lineStr[0], 0, BUFFER_SIZE);
        memcpy(lineStr[0], value, strlen(value));
        text_layer_set_text(next, lineStr[0]);
    }

    makeAnimationsForLayers(line, current, next);
}

// Check if a line's displayed value differs from nextValue
bool needToUpdateLine(Line *line, char lineStr[2][BUFFER_SIZE], char *nextValue) {
    GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
    char *currentStr = (rect.origin.x == 0) ? lineStr[0] : lineStr[1];

    return memcmp(currentStr, nextValue, strlen(nextValue)) != 0 ||
           (strlen(nextValue) == 0 && strlen(currentStr) != 0);
}

// ---------------------------------------------------------------------------
// Date / time formatting
// ---------------------------------------------------------------------------

// Format the date as a displayable string, stripping leading zeros
void date_to_string(struct tm *t, char *line, size_t length) {
    memset(line, 0, length);
    strftime(line, length, DateFormat, t);
    if (line[3] == '0') memmove(&line[3], &line[4], length - 4);
    if (line[0] == '0') memmove(&line[0], &line[1], length - 1);
}

// Format the weekday abbreviation as a lowercase displayable string
void day_of_week(struct tm *t, char *line, size_t length) {
    memset(line, 0, length);
    strftime(line, length, "%a", t);
    line[0] += 32;  // lowercase first char

#if DateTruncTo2Char
    line[2] = 0;
#endif
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

#if DateSeparatorLine
void lineDrawLayerCallback(Layer *me, GContext *ctx) {
    GRect bounds = layer_get_bounds(me);
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx,
        GPoint(layout.sep_inset, layout.sep_y),
        GPoint(bounds.size.w - layout.sep_inset, layout.sep_y));
    graphics_draw_line(ctx,
        GPoint(layout.sep_inset, layout.sep_y + 1),
        GPoint(bounds.size.w - layout.sep_inset, layout.sep_y + 1));
}
#endif

// Update all display lines from the given time
void display_time(struct tm *t) {
    char textLine1[BUFFER_SIZE];
    char textLine2[BUFFER_SIZE];
    char textLine3[BUFFER_SIZE];
    char textLine4[BUFFER_SIZE];
    char textLine5[BUFFER_SIZE];

    time_to_3words(t->tm_hour, t->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE, layout.screen_width < 200);
    day_of_week(t, textLine4, BUFFER_SIZE);
    date_to_string(t, textLine5, BUFFER_SIZE);

    if (needToUpdateLine(&line1, line1Str, textLine1)) updateLineTo(&line1, line1Str, textLine1);
    if (needToUpdateLine(&line2, line2Str, textLine2)) updateLineTo(&line2, line2Str, textLine2);
    if (needToUpdateLine(&line3, line3Str, textLine3)) updateLineTo(&line3, line3Str, textLine3);

    if (needToUpdateLine(&line4, line4Str, textLine4)) {
        memset(line4Str[0], 0, BUFFER_SIZE);
        memcpy(line4Str[0], textLine4, strlen(textLine4));
        text_layer_set_text(line4.currentLayer, line4Str[0]);
    }
    if (needToUpdateLine(&line5, line5Str, textLine5)) {
        memset(line5Str[0], 0, BUFFER_SIZE);
        memcpy(line5Str[0], textLine5, strlen(textLine5));
        text_layer_set_text(line5.currentLayer, line5Str[0]);
    }
}

// Set the initial display without animation
void display_initial_time(struct tm *t) {
    time_to_3words(t->tm_hour, t->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE, layout.screen_width < 200);
    day_of_week(t, line4Str[0], BUFFER_SIZE);
    date_to_string(t, line5Str[0], BUFFER_SIZE);

    text_layer_set_text(line1.currentLayer, line1Str[0]);
    text_layer_set_text(line2.currentLayer, line2Str[0]);
    text_layer_set_text(line3.currentLayer, line3Str[0]);
    text_layer_set_text(line4.currentLayer, line4Str[0]);
    text_layer_set_text(line5.currentLayer, line5Str[0]);
}

static void configure_text_layer(TextLayer *layer, const char *font_key, bool right) {
    text_layer_set_font(layer, fonts_get_system_font(font_key));
    text_layer_set_text_color(layer, GColorWhite);
    text_layer_set_background_color(layer, GColorClear);
    text_layer_set_text_alignment(layer, right ? GTextAlignmentRight : GTextAlignmentLeft);
}

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------

/**
 * Enable DEBUG above to turn the watchface into a standard app and step
 * through time with the up/down buttons.
 */
#if DEBUG

static void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    debug_time.tm_min += 1;
    if (debug_time.tm_min >= 60) {
        debug_time.tm_min = 0;
        if (++debug_time.tm_hour >= 24) debug_time.tm_hour = 0;
    }
    display_time(&debug_time);
}

static void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    debug_time.tm_min -= 1;
    if (debug_time.tm_min < 0) {
        debug_time.tm_min = 59;
        debug_time.tm_hour -= 1;
    }
    display_time(&debug_time);
}

static void click_config_provider(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP,   100, up_single_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_single_click_handler);
}

#endif

// ---------------------------------------------------------------------------
// App lifecycle
// ---------------------------------------------------------------------------

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    display_time(tick_time);
}

static void handle_init(void) {
    window = window_create();
    window_stack_push(window, true);
    window_set_background_color(window, GColorBlack);

    Layer *root_layer = window_get_root_layer(window);
    layout_config_init(&layout, layer_get_bounds(root_layer));

    // Time rows — current layer starts on-screen (x=0), next parks off-screen right
    line1.currentLayer = text_layer_create(GRect(0,                   layout.line1_y, layout.screen_width, layout.line_height));
    line1.nextLayer    = text_layer_create(GRect(layout.screen_width, layout.line1_y, layout.screen_width, layout.line_height));
    configure_text_layer(line1.currentLayer, layout.time_bold_font, false);
    configure_text_layer(line1.nextLayer,    layout.time_bold_font, false);

    line2.currentLayer = text_layer_create(GRect(0,                   layout.line2_y, layout.screen_width, layout.line_height));
    line2.nextLayer    = text_layer_create(GRect(layout.screen_width, layout.line2_y, layout.screen_width, layout.line_height));
    configure_text_layer(line2.currentLayer, layout.time_light_font, false);
    configure_text_layer(line2.nextLayer,    layout.time_light_font, false);

    line3.currentLayer = text_layer_create(GRect(0,                   layout.line3_y, layout.screen_width, layout.line_height));
    line3.nextLayer    = text_layer_create(GRect(layout.screen_width, layout.line3_y, layout.screen_width, layout.line_height));
    configure_text_layer(line3.currentLayer, layout.time_light_font, false);
    configure_text_layer(line3.nextLayer,    layout.time_light_font, false);

#if DateSeparatorLine
    lineDrawLayer = layer_create(layer_get_bounds(root_layer));
    layer_set_update_proc(lineDrawLayer, lineDrawLayerCallback);
#endif

    // Weekday and date rows (no animation — static layers only)
    line4.currentLayer = text_layer_create(GRect(layout.weekday_x, layout.weekday_y, layout.weekday_w, layout.line_height));
    configure_text_layer(line4.currentLayer, layout.time_light_font, layout.weekday_right);

    line5.currentLayer = text_layer_create(GRect(layout.date_x, layout.date_y, layout.date_w, layout.line_height));
    configure_text_layer(line5.currentLayer, layout.date_font, layout.date_right);

    // Show current time immediately
    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    display_initial_time(tick_time);

#if DEBUG
    debug_time = *tick_time;
#endif

    layer_add_child(root_layer, text_layer_get_layer(line1.currentLayer));
    layer_add_child(root_layer, text_layer_get_layer(line1.nextLayer));
    layer_add_child(root_layer, text_layer_get_layer(line2.currentLayer));
    layer_add_child(root_layer, text_layer_get_layer(line2.nextLayer));
    layer_add_child(root_layer, text_layer_get_layer(line3.currentLayer));
    layer_add_child(root_layer, text_layer_get_layer(line3.nextLayer));
#if DateSeparatorLine
    layer_add_child(root_layer, lineDrawLayer);
#endif
    layer_add_child(root_layer, text_layer_get_layer(line4.currentLayer));
    layer_add_child(root_layer, text_layer_get_layer(line5.currentLayer));

#if DEBUG
    window_set_click_config_provider(window, click_config_provider);
#endif

#if !DEBUG
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
#endif
}

static void handle_deinit(void) {
#if !DEBUG
    tick_timer_service_unsubscribe();
#endif

    text_layer_destroy(line1.currentLayer);
    text_layer_destroy(line1.nextLayer);
    text_layer_destroy(line2.currentLayer);
    text_layer_destroy(line2.nextLayer);
    text_layer_destroy(line3.currentLayer);
    text_layer_destroy(line3.nextLayer);
    text_layer_destroy(line4.currentLayer);
    text_layer_destroy(line5.currentLayer);

#if DateSeparatorLine
    layer_destroy(lineDrawLayer);
#endif

    window_destroy(window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
    return 0;
}
