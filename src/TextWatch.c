#include "pebble.h"

#include "num2words-en.h"
#include "config.h"

#define DEBUG false
#define BUFFER_SIZE 44

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

// Reset off-screen layer position after animation completes
void animationStoppedHandler(Animation *animation, bool finished, void *context) {
    TextLayer *current = (TextLayer *)context;
    GRect rect = layer_get_frame(text_layer_get_layer(current));
    rect.origin.x = 144;
    layer_set_frame(text_layer_get_layer(current), rect);
}

// Animate a line transition by sliding current out and next in
void makeAnimationsForLayers(Line *line, TextLayer *current, TextLayer *next) {
    GRect rect = layer_get_frame(text_layer_get_layer(next));
    rect.origin.x -= 144;

    line->nextAnimation = property_animation_create_layer_frame(text_layer_get_layer(next), NULL, &rect);
    animation_set_duration((Animation *)line->nextAnimation, 400);
    animation_set_curve((Animation *)line->nextAnimation, AnimationCurveEaseOut);
    animation_schedule((Animation *)line->nextAnimation);

    GRect rect2 = layer_get_frame(text_layer_get_layer(current));
    rect2.origin.x -= 144;

    line->currentAnimation = property_animation_create_layer_frame(text_layer_get_layer(current), NULL, &rect2);
    animation_set_duration((Animation *)line->currentAnimation, 400);
    animation_set_curve((Animation *)line->currentAnimation, AnimationCurveEaseOut);
    animation_set_handlers((Animation *)line->currentAnimation, (AnimationHandlers) {
        .stopped = animationStoppedHandler
    }, current);
    animation_schedule((Animation *)line->currentAnimation);
}

// Update a line with a sliding animation to the new value
void updateLineTo(Line *line, char lineStr[2][BUFFER_SIZE], char *value) {
    TextLayer *next, *current;

    GRect rect = layer_get_frame(text_layer_get_layer(line->currentLayer));
    current = (rect.origin.x == 0) ? line->currentLayer : line->nextLayer;
    next = (current == line->currentLayer) ? line->nextLayer : line->currentLayer;

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

#if DateSeparatorLine
void lineDrawLayerCallback(Layer *me, GContext *ctx) {
    (void)me;
    int16_t w = layer_get_bounds(window_get_root_layer(window)).size.w;
    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_draw_line(ctx, GPoint(lineInset, lineVOffset),
                            GPoint(w - lineInset, lineVOffset));
    graphics_draw_line(ctx, GPoint(lineInset, lineVOffset + 1),
                            GPoint(w - lineInset, lineVOffset + 1));
}
#endif

// Update all display lines from the given time
void display_time(struct tm *t) {
    char textLine1[BUFFER_SIZE];
    char textLine2[BUFFER_SIZE];
    char textLine3[BUFFER_SIZE];
    char textLine4[BUFFER_SIZE];
    char textLine5[BUFFER_SIZE];

    time_to_3words(t->tm_hour, t->tm_min, textLine1, textLine2, textLine3, BUFFER_SIZE);
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
    time_to_3words(t->tm_hour, t->tm_min, line1Str[0], line2Str[0], line3Str[0], BUFFER_SIZE);
    day_of_week(t, line4Str[0], BUFFER_SIZE);
    date_to_string(t, line5Str[0], BUFFER_SIZE);

    text_layer_set_text(line1.currentLayer, line1Str[0]);
    text_layer_set_text(line2.currentLayer, line2Str[0]);
    text_layer_set_text(line3.currentLayer, line3Str[0]);
    text_layer_set_text(line4.currentLayer, line4Str[0]);
    text_layer_set_text(line5.currentLayer, line5Str[0]);
}

static void configureBoldLayer(TextLayer *textlayer, bool right) {
    text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_color(textlayer, GColorWhite);
    text_layer_set_background_color(textlayer, GColorClear);
    text_layer_set_text_alignment(textlayer, right ? GTextAlignmentRight : GTextAlignmentLeft);
}

static void configureLightLayer(TextLayer *textlayer, bool right) {
    text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
    text_layer_set_text_color(textlayer, GColorWhite);
    text_layer_set_background_color(textlayer, GColorClear);
    text_layer_set_text_alignment(textlayer, right ? GTextAlignmentRight : GTextAlignmentLeft);
}

static void configureDateLayer(TextLayer *textlayer, bool right) {
    text_layer_set_font(textlayer, fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));
    text_layer_set_text_color(textlayer, GColorWhite);
    text_layer_set_background_color(textlayer, GColorClear);
    text_layer_set_text_alignment(textlayer, right ? GTextAlignmentRight : GTextAlignmentLeft);
}

/**
 * Debug handlers: enable DEBUG macro above to turn the watchface into a standard app
 * and step through time with the up/down buttons.
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

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    display_time(tick_time);
}

static void handle_init(void) {
    window = window_create();
    window_stack_push(window, true);
    window_set_background_color(window, GColorBlack);

    Layer *root_layer = window_get_root_layer(window);

    // Line 1 — bold
    line1.currentLayer = text_layer_create(GRect(0,   TextLineVOffset,      144, 50));
    line1.nextLayer    = text_layer_create(GRect(144, TextLineVOffset,      144, 50));
    configureBoldLayer(line1.currentLayer, false);
    configureBoldLayer(line1.nextLayer,    false);

    // Line 2 — light
    line2.currentLayer = text_layer_create(GRect(0,   37 + TextLineVOffset, 144, 50));
    line2.nextLayer    = text_layer_create(GRect(144, 37 + TextLineVOffset, 144, 50));
    configureLightLayer(line2.currentLayer, false);
    configureLightLayer(line2.nextLayer,    false);

    // Line 3 — light
    line3.currentLayer = text_layer_create(GRect(0,   74 + TextLineVOffset, 144, 50));
    line3.nextLayer    = text_layer_create(GRect(144, 74 + TextLineVOffset, 144, 50));
    configureLightLayer(line3.currentLayer, false);
    configureLightLayer(line3.nextLayer,    false);

#if DateSeparatorLine
    lineDrawLayer = layer_create(layer_get_bounds(root_layer));
    layer_set_update_proc(lineDrawLayer, lineDrawLayerCallback);
#endif

    // Line 4 — weekday
    line4.currentLayer = text_layer_create(
        GRect(WeekdayHStart, DateVOffset, WeekdayHStop - WeekdayHStart, 50));
    configureLightLayer(line4.currentLayer, WeekdayRightJust);

    // Line 5 — date
    line5.currentLayer = text_layer_create(
        GRect(DateHStart, 8 + DateVOffset, DateHStop - DateHStart, 50));
    configureDateLayer(line5.currentLayer, DateRightJust);

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

    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void handle_deinit(void) {
    tick_timer_service_unsubscribe();

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
