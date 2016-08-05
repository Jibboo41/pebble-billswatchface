#include <pebble.h>


static Window *s_main_window;
static TextLayer *s_time_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static TextLayer *s_step_layer;
static Layer *s_canvas_batt_layer;
static int battery_charge_percent;

/**
 * Update the time text layer. 
 */
static void update_time() 
{
   // Get a tm structure
   time_t temp = time(NULL);
   struct tm *tick_time = localtime(&temp);

   // Write the current hours and minutes into a buffer
   static char s_buffer[8];
   strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
           "%H:%M" : "%I:%M", tick_time);
   
  // Display this time on the Time TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

/**
 * Update the step text layer. 
 */
static void update_step_count()
{
   HealthMetric metric = HealthMetricStepCount;
   time_t start = time_start_of_today();
   time_t end = time(NULL);
   
   // Check the metric has data available for today
   HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, 
      start, end);
   
   if(mask & HealthServiceAccessibilityMaskAvailable) 
   {
      // Data is available!
      int steps =  (int)health_service_sum_today(metric);
      APP_LOG(APP_LOG_LEVEL_INFO, "Steps today: %d", steps);
      
      // Write the steps into a buffer
      static char s_buffer[6]; 
      snprintf(s_buffer, sizeof(s_buffer),"%d", steps);  
      
      // Display this time on the step TextLayer
      text_layer_set_text(s_step_layer, s_buffer);

   } 
   else 
   {
     // No data recorded yet today
     APP_LOG(APP_LOG_LEVEL_ERROR, "Data unavailable!");
   }
}

// Update the battery layer
static void canvas_batt_update_proc(Layer *layer, GContext *ctx) 
{
   // Set the line color
   graphics_context_set_stroke_color(ctx, GColorBlue);

   // Set the fill color
   graphics_context_set_fill_color(ctx, GColorBlue);

   // Set the stroke width (must be an odd integer value)
   graphics_context_set_stroke_width(ctx, 3);

   // Disable antialiasing (enabled by default where available)
   //graphics_context_set_antialiased(ctx, false);
   
   GPoint start = GPoint(5, 123);
//    GPoint end = GPoint(battery_charge_percent, 123);
   
   int endX = (175-5) * ((double)battery_charge_percent/100.0);
   GPoint end = GPoint(endX, 123);
   // Draw a line
   graphics_draw_line(ctx, start, end);
}

/**)
 * Time change handler
 */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) 
{
   update_time();
}

/**
 * Bluetooth connection handler
 */ 
static void app_connection_handler(bool connected) 
{
   APP_LOG(APP_LOG_LEVEL_INFO, "Pebble app %sconnected", connected ? "" : "dis");
   
   // TODO is this the best way to change the background image?
   if (s_background_bitmap) 
   {
		gbitmap_destroy(s_background_bitmap);
		s_background_bitmap = NULL;
   }
   
   s_background_bitmap = gbitmap_create_with_resource(
                         connected ?  
                         RESOURCE_ID_BILLS_BACKGROUND : 
                         RESOURCE_ID_BILLS_BACKGROUND_NO_BT);
   
   bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
   
   bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	// layer_set_hidden(bitmap_layer_get_layer(background_layer), false);
   layer_mark_dirty(bitmap_layer_get_layer(s_background_layer)); 
   
   vibes_short_pulse();
}

/**
 * Health handler
 */ 
static void health_handler(HealthEventType event, void *context) 
{
   // Which type of event occurred?
   switch(event) 
   {
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

/**
 * Battery handler
 */
static void handle_battery(BatteryChargeState charge_state) 
{
   battery_charge_percent =  charge_state.charge_percent;
   if (charge_state.is_charging) 
   {
      //snprintf(battery_text, sizeof(battery_text), "charging");
   } 
   else 
   {
      //snprintf(battery_text, sizeof(battery_text), "%d%% charged", charge_state.charge_percent);
   }
   
   layer_mark_dirty(s_canvas_batt_layer);
}

/**
 * Main window load.
 */
static void main_window_load(Window *window) 
{
   // Get information about the Window
   Layer *window_layer = window_get_root_layer(window);
   GRect bounds = layer_get_bounds(window_layer);

   //=========================================================================
   // Background layer
   //=========================================================================   
   // Create GBitmap
   s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BILLS_BACKGROUND);
   
   // Create BitmapLayer to display the GBitmap
   s_background_layer = bitmap_layer_create(bounds);
   
   // Set the bitmap onto the layer and add to the window
   bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
   layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
   
   //=========================================================================
   // Time layer
   //=========================================================================
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
   
   //=========================================================================
   // Step layer
   //=========================================================================
   // Create the TextLayer with specific bounds
   s_step_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(100, 52), bounds.size.w, 50));

   // Improve the layout
   text_layer_set_background_color(s_step_layer, GColorClear);
   text_layer_set_text_color(s_step_layer, GColorWhite);
   text_layer_set_font(s_step_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
   text_layer_set_text_alignment(s_step_layer, GTextAlignmentCenter);

   // Add it as a child layer to the Window's root layer
   layer_add_child(window_layer, text_layer_get_layer(s_step_layer));
   
  //=========================================================================
   // Canvas battery layer
   //=========================================================================
   s_canvas_batt_layer = layer_create(bounds);
   
   // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_batt_layer, canvas_batt_update_proc);

   // Add to Window
   layer_add_child(window_get_root_layer(window), s_canvas_batt_layer);
}

/**
 * Main window unload.
 */
static void main_window_unload(Window *window) 
{
   // Destroy step layer
   text_layer_destroy(s_step_layer);
   
   // Destroy time layer
   text_layer_destroy(s_time_layer);
   
   // Destroy GBitmap background image
   gbitmap_destroy(s_background_bitmap);

   // Destroy background layer
   bitmap_layer_destroy(s_background_layer);
}

/**
 * Initialize
 */
static void init() 
{
   // Create main Window element and assign to pointer
   s_main_window = window_create();

   // Set handlers to manage the elements inside the Window
   window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
   });

   // Subscribe for time updates.
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

   // Subscribe for bluetooth updates.
  connection_service_subscribe((ConnectionHandlers) {
     .pebble_app_connection_handler = app_connection_handler
   }); 
   
   // Subscribe for health updates.
   health_service_events_subscribe(health_handler, NULL); 
   
   // Subscribe for battery updates.
   battery_state_service_subscribe(handle_battery);
   
   // Show the Window on the watch, with animated=true
   window_stack_push(s_main_window, true);
   
   // Display the time
   update_time();
   
   // Display the step count
   update_step_count();
   
   // Display the battery level
   handle_battery(battery_state_service_peek());
}

/**
 * De-initialize
 */
static void deinit() 
{
  // Destroy Window
  window_destroy(s_main_window);
}

/**
 * Main
 */
int main(void) 
{
  init();
  app_event_loop();
  deinit();
}