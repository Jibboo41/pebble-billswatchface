#include <pebble.h>


static Window *s_main_window;
static TextLayer *s_time_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static TextLayer *s_step_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
 
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

      // Create GBitmap
   s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BILLS_BACKGROUND);
   
   // Create BitmapLayer to display the GBitmap
   s_background_layer = bitmap_layer_create(bounds);
   
   // Set the bitmap onto the layer and add to the window
   bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
   layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
   
   
  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(115, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
   
   // step layer
     // Create the TextLayer with specific bounds
  s_step_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(100, 52), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_step_layer, GColorClear);
  text_layer_set_text_color(s_step_layer, GColorWhite);
  text_layer_set_font(s_step_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_step_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_step_layer));
   
}

static void main_window_unload(Window *window) {
   text_layer_destroy(s_time_layer);
   // Destroy GBitmap
gbitmap_destroy(s_background_bitmap);

// Destroy BitmapLayer
bitmap_layer_destroy(s_background_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
update_time();
}

static void app_connection_handler(bool connected) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Pebble app %sconnected", connected ? "" : "dis");
   	    if (s_background_bitmap) {
		gbitmap_destroy(s_background_bitmap);
		s_background_bitmap = NULL;
    }
   
     s_background_bitmap = gbitmap_create_with_resource(connected? 
                                                        RESOURCE_ID_BILLS_BACKGROUND : RESOURCE_ID_BILLS_BACKGROUND_NO_BT);
     bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
   
  		bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
		//layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
		layer_mark_dirty(bitmap_layer_get_layer(s_background_layer)); 
   
   vibes_short_pulse();
}

static void update_step_count()
{
   HealthMetric metric = HealthMetricStepCount;
   time_t start = time_start_of_today();
   time_t end = time(NULL);
   
   // Check the metric has data available for today
   HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, 
     start, end);
   
   if(mask & HealthServiceAccessibilityMaskAvailable) {
     // Data is available!
      int steps =  (int)health_service_sum_today(metric);
     APP_LOG(APP_LOG_LEVEL_INFO, "Steps today: %d", steps);
      
        // Write the steps into a buffer


static char s_buffer[6]; 

snprintf(s_buffer,6,"%d", steps);  
      
  // Display this time on the TextLayer
  text_layer_set_text(s_step_layer, s_buffer);
      //text_layer_set_text(s_step_layer, "temp");
   } else {
     // No data recorded yet today
     APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
   }
}
static void health_handler(HealthEventType event, void *context) {
  // Which type of event occurred?
  switch(event) {
    case HealthEventSignificantUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, 
              "New HealthService HealthEventSignificantUpdate event");
     update_step_count();
      break;
    case HealthEventMovementUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, 
              "New HealthService HealthEventMovementUpdate event");
     update_step_count();
      break;
    case HealthEventSleepUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, 
              "New HealthService HealthEventSleepUpdate event");
      break;
  }
}

static void init() {
 // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  connection_service_subscribe((ConnectionHandlers) {
  .pebble_app_connection_handler = app_connection_handler
}); 
   
  health_service_events_subscribe(health_handler, NULL); 
   
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
   
      // Make sure the time is displayed from the start
   update_time();
   update_step_count();
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}