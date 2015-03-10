#include "pebble.h"
#include "gcolor_definitions.h"

static Window *window;
static Layer *s_simple_bg_layer, *s_hands_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static TextLayer *s_time_layer;

static GFont s_time_font;

static char s_num_buffer[4];

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(55, 18);
  int16_t second_hand_length = bounds.size.w / 11;
  int16_t minute_hand_length = bounds.size.w / 12;
  int16_t hour_hand_length = bounds.size.w / 13;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  int32_t hour_angle = (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6);
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  GPoint minute_hand = {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)minute_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)minute_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  GPoint hour_hand = {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)hour_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  
  // second hand
  graphics_context_set_stroke_color(ctx, GColorRajah);
  graphics_draw_line(ctx, second_hand, center);
  
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, minute_hand, center);
  graphics_draw_line(ctx, hour_hand, center);
}

// Handles updating time
static void update_time() {
  // Get time
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create buffer for time
  static char buffer[] = "00:00";

  // Write time as 24h or 12h format onto the buffer
  if(clock_is_24h_style() == true) {
    // 24h format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // 12h format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display on Time Layer
  text_layer_set_text(s_time_layer, buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
  update_time();
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the font
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_OPEN_SANS_12));



  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);
  
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
  s_background_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

 // Create time layer. Add time font to layer. Apply it to window_layer
  s_time_layer = text_layer_create(GRect(55, 32, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  // Display the time immediately
  update_time();

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);
  // Unload the font
  fonts_unload_custom_font(s_time_font);
  
  // Destroy the time layer
  text_layer_destroy(s_time_layer);
  
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_background_bitmap);
  
  layer_destroy(s_hands_layer);
}

static void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);
  
  s_num_buffer[0] = '\0';

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}