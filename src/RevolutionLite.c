// Copyright (c) 2013 Douwe Maan <http://www.douwemaan.com/>
// The above copyright notice shall be included in all copies or substantial portions of the program.

// Envisioned as a watchface by Jean-Noël Mattern
// Based on the display of the Freebox Revolution, which was designed by Philippe Starck.

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xAF, 0x25, 0x74, 0x3A, 0x2C, 0xC9, 0x4F, 0x16, 0x92, 0x6B, 0x87, 0x97, 0x44, 0x0A, 0xC8, 0xA1 }
PBL_APP_INFO(MY_UUID,
             "RevolutionLite", "Nate Gantt",
             1, 5, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

// Settings
#define USE_AMERICAN_DATE_FORMAT   true
#define HAVE_NO_ZEROS   true

// MumboJumbo Numbers
#define SCREEN_WIDTH        144
#define SCREEN_HEIGHT       168

#define TIME_IMAGE_WIDTH    70
#define TIME_IMAGE_HEIGHT   70

#define DATE_IMAGE_WIDTH    20
#define DATE_IMAGE_HEIGHT   20

#define YEAR_IMAGE_WIDTH  10
#define YEAR_IMAGE_HEIGHT 10

#define DAY_IMAGE_WIDTH     20
#define DAY_IMAGE_HEIGHT    10

#define MARGIN              1
#define TIME_SLOT_SPACE     2
#define DATE_PART_SPACE     4


// Images
#define NUMBER_OF_TIME_IMAGES 10
const int TIME_IMAGE_RESOURCE_IDS[NUMBER_OF_TIME_IMAGES] = {
  RESOURCE_ID_IMAGE_TIME_0, 
  RESOURCE_ID_IMAGE_TIME_1, RESOURCE_ID_IMAGE_TIME_2, RESOURCE_ID_IMAGE_TIME_3, 
  RESOURCE_ID_IMAGE_TIME_4, RESOURCE_ID_IMAGE_TIME_5, RESOURCE_ID_IMAGE_TIME_6, 
  RESOURCE_ID_IMAGE_TIME_7, RESOURCE_ID_IMAGE_TIME_8, RESOURCE_ID_IMAGE_TIME_9
};

#define NUMBER_OF_DATE_IMAGES 10
const int DATE_IMAGE_RESOURCE_IDS[NUMBER_OF_DATE_IMAGES] = {
  RESOURCE_ID_IMAGE_DATE_0, 
  RESOURCE_ID_IMAGE_DATE_1, RESOURCE_ID_IMAGE_DATE_2, RESOURCE_ID_IMAGE_DATE_3, 
  RESOURCE_ID_IMAGE_DATE_4, RESOURCE_ID_IMAGE_DATE_5, RESOURCE_ID_IMAGE_DATE_6, 
  RESOURCE_ID_IMAGE_DATE_7, RESOURCE_ID_IMAGE_DATE_8, RESOURCE_ID_IMAGE_DATE_9
};

#define NUMBER_OF_YEAR_IMAGES 10
const int YEAR_IMAGE_RESOURCE_IDS[NUMBER_OF_YEAR_IMAGES] = {
  RESOURCE_ID_IMAGE_YEAR_0, 
  RESOURCE_ID_IMAGE_YEAR_1, RESOURCE_ID_IMAGE_YEAR_2, RESOURCE_ID_IMAGE_YEAR_3, 
  RESOURCE_ID_IMAGE_YEAR_4, RESOURCE_ID_IMAGE_YEAR_5, RESOURCE_ID_IMAGE_YEAR_6, 
  RESOURCE_ID_IMAGE_YEAR_7, RESOURCE_ID_IMAGE_YEAR_8, RESOURCE_ID_IMAGE_YEAR_9
};

#define NUMBER_OF_DAY_IMAGES  7
const int DAY_IMAGE_RESOURCE_IDS[NUMBER_OF_DAY_IMAGES] = {
  RESOURCE_ID_IMAGE_DAY_0, RESOURCE_ID_IMAGE_DAY_1, RESOURCE_ID_IMAGE_DAY_2, 
  RESOURCE_ID_IMAGE_DAY_3, RESOURCE_ID_IMAGE_DAY_4, RESOURCE_ID_IMAGE_DAY_5, 
  RESOURCE_ID_IMAGE_DAY_6
};

// Main
Window window;
Layer date_container_layer;
Layer year_layer;

#define EMPTY_SLOT -1

// Time
typedef struct TimeSlot {
  int           number;
  BmpContainer  image_container;
  Layer         layer;
  int           state;
  int           new_state;
} TimeSlot;

#define NUMBER_OF_TIME_SLOTS 4
TimeSlot time_slots[NUMBER_OF_TIME_SLOTS];


// Date
typedef struct DateSlot {
  int           number;
  BmpContainer  image_container;
  int           state;
} DateSlot;

#define NUMBER_OF_DATE_SLOTS 4
Layer date_layer;
DateSlot date_slots[NUMBER_OF_DATE_SLOTS];

// Year
#define NUMBER_OF_YEAR_SLOTS 2
BmpContainer year_image_containers[NUMBER_OF_YEAR_SLOTS];
bool year_images_loaded;

// Day
typedef struct DaySlot {
  BmpContainer  image_container;
  Layer         layer;
  bool          loaded;
} DaySlot;
DaySlot day_slot;


// Display
void display_time(PblTm *tick_time);
void display_date(PblTm *tick_time);
void display_year(PblTm *tick_time);
void display_day(PblTm *tick_time);
void draw_date_container(Layer *layer, GContext *ctx);

// Time
void display_time_value(int value, int row_number);
void update_time_slot(TimeSlot *time_slot, int digit_value);
void load_digit_image_into_time_slot(TimeSlot *time_slot, int digit_value);
void unload_digit_image_from_time_slot(TimeSlot *time_slot);
GRect frame_for_time_slot(TimeSlot *time_slot);

// Date
void display_date_value(int value, int part_number);
void update_date_slot(DateSlot *date_slot, int digit_value);
void load_digit_image_into_date_slot(DateSlot *date_slot, int digit_value);
void unload_digit_image_from_date_slot(DateSlot *date_slot);
GRect frame_for_date_slot(DateSlot *date_slot);

// Handlers
void pbl_main(void *params);
void handle_init(AppContextRef ctx);
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *event);
void handle_deinit(AppContextRef ctx);


// Display
void display_time(PblTm *tick_time) {
  int hour = tick_time->tm_hour;

  if (!clock_is_24h_style()) {
    hour = hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  display_time_value(hour,              0);
  display_time_value(tick_time->tm_min, 1);
}

void display_date(PblTm *tick_time) {
  int day   = tick_time->tm_mday;
  int month = tick_time->tm_mon + 1;

#if USE_AMERICAN_DATE_FORMAT
  display_date_value(month, 0);
  display_date_value(day,   1);
#else
  display_date_value(day,   0);
  display_date_value(month, 1);
#endif
}

void display_year(PblTm *tick_time) {
  if (year_images_loaded) {
    for (int i = 0; i < NUMBER_OF_YEAR_SLOTS; i++) {
      layer_remove_from_parent(&year_image_containers[i].layer.layer);
      bmp_deinit_container(&year_image_containers[i]);
    }
  }

  int year = tick_time->tm_year;

  year = year % 100; // Maximum of two digits per row.

  for (int column_number = 1; column_number >= 0; column_number--) {
    BmpContainer *image_container = &year_image_containers[column_number];

    GRect frame = GRect(
      column_number * (YEAR_IMAGE_WIDTH + MARGIN), 
      0, 
      YEAR_IMAGE_WIDTH, 
      YEAR_IMAGE_HEIGHT
    );

    bmp_init_container(YEAR_IMAGE_RESOURCE_IDS[year % 10], image_container);
    layer_set_frame(&image_container->layer.layer, frame);
    layer_add_child(&year_layer, &image_container->layer.layer);
    
    year = year / 10;
  }
  year_images_loaded = true;
}

void display_day(PblTm *tick_time) {
  BmpContainer *image_container = &day_slot.image_container;

  if (day_slot.loaded) {
    layer_remove_from_parent(&image_container->layer.layer);
    bmp_deinit_container(image_container);
  }

  bmp_init_container(DAY_IMAGE_RESOURCE_IDS[tick_time->tm_wday], image_container);
  layer_add_child(&day_slot.layer, &image_container->layer.layer);

  day_slot.loaded = true;
}

void draw_date_container(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, layer->bounds.size.w, layer->bounds.size.h), 0, GCornerNone);
}

// Time
void display_time_value(int value, int row_number) {
  value = value % 100; // Maximum of two digits per row.

  for (int column_number = 1; column_number >= 0; column_number--) {
    int time_slot_number = (row_number * 2) + column_number;

    TimeSlot *time_slot = &time_slots[time_slot_number];

    update_time_slot(time_slot, value % 10);

    value = value / 10;
  }
}

void update_time_slot(TimeSlot *time_slot, int digit_value) {
 
  if (time_slot->state == digit_value) {
    return;
  }
 
  unload_digit_image_from_time_slot(time_slot);

  #if HAVE_NO_ZEROS
  if (time_slot->number !=0 || digit_value !=0) {
  load_digit_image_into_time_slot(time_slot, digit_value);
  }
  #else
  load_digit_image_into_time_slot(time_slot, digit_value);
  #endif
}
 
 
void load_digit_image_into_time_slot(TimeSlot *time_slot, int digit_value) {
  if (digit_value < 0 || digit_value > 9) {
    return;
  }
 
  if (time_slot->state != EMPTY_SLOT) {
    return;
  }
 
  time_slot->state = digit_value;
 
  bmp_init_container(TIME_IMAGE_RESOURCE_IDS[digit_value], &time_slot->image_container);
  layer_add_child(&time_slot->layer, &time_slot->image_container.layer.layer);
}
 
void unload_digit_image_from_time_slot(TimeSlot *time_slot) {
  if (time_slot->state == EMPTY_SLOT) {
    return;
  }
 
  layer_remove_from_parent(&time_slot->image_container.layer.layer);
  bmp_deinit_container(&time_slot->image_container);
 
  time_slot->state = EMPTY_SLOT;;
}
 
GRect frame_for_time_slot(TimeSlot *time_slot) {
  int x = MARGIN + (time_slot->number % 2) * (TIME_IMAGE_WIDTH + TIME_SLOT_SPACE);
  int y = MARGIN + (time_slot->number / 2) * (TIME_IMAGE_HEIGHT + TIME_SLOT_SPACE);
 
  return GRect(x, y, TIME_IMAGE_WIDTH, TIME_IMAGE_HEIGHT);
}


// Date
void display_date_value(int value, int part_number) {
  value = value % 100; // Maximum of two digits per row.

  for (int column_number = 1; column_number >= 0; column_number--) {
    int date_slot_number = (part_number * 2) + column_number;

    DateSlot *date_slot = &date_slots[date_slot_number];

    update_date_slot(date_slot, value % 10);

    value = value / 10;
  }
}

void update_date_slot(DateSlot *date_slot, int digit_value) {
  if (date_slot->state == digit_value) {
    return;
  }

  unload_digit_image_from_date_slot(date_slot);
  load_digit_image_into_date_slot(date_slot, digit_value);
}

void load_digit_image_into_date_slot(DateSlot *date_slot, int digit_value) {
  if (digit_value < 0 || digit_value > 9) {
    return;
  }

  if (date_slot->state != EMPTY_SLOT) {
    return;
  }

  date_slot->state = digit_value;

  BmpContainer *image_container = &date_slot->image_container;

  bmp_init_container(DATE_IMAGE_RESOURCE_IDS[digit_value], image_container);
  layer_set_frame(&image_container->layer.layer, frame_for_date_slot(date_slot));
  layer_add_child(&date_layer, &image_container->layer.layer);
}

void unload_digit_image_from_date_slot(DateSlot *date_slot) {
  if (date_slot->state == EMPTY_SLOT) {
    return;
  }

  BmpContainer *image_container = &date_slot->image_container;

  layer_remove_from_parent(&image_container->layer.layer);
  bmp_deinit_container(image_container);

  date_slot->state = EMPTY_SLOT;
}

GRect frame_for_date_slot(DateSlot *date_slot) {
  int x = date_slot->number * (DATE_IMAGE_WIDTH + MARGIN);
  if (date_slot->number >= 2) {
    x += 3; // 3 extra pixels of space between the day and month
  }

  return GRect(x, 0, DATE_IMAGE_WIDTH, DATE_IMAGE_HEIGHT);
}


// Handlers
void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler   = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {
      .tick_handler = &handle_minute_tick,
      .tick_units   = MINUTE_UNIT
    }
  };

  app_event_loop(params, &handlers);
}


void handle_init(AppContextRef ctx) {
  window_init(&window, "RevolutionLite");
  window_stack_push(&window, true /* Animated */);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  // Time slots
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    TimeSlot *time_slot = &time_slots[i];
    time_slot->number     = i;
    time_slot->state      = EMPTY_SLOT;
    time_slot->new_state  = EMPTY_SLOT;
  }

  // Date slots
  for (int i = 0; i < NUMBER_OF_DATE_SLOTS; i++) {
    DateSlot *date_slot = &date_slots[i];
    date_slot->number = i;
    date_slot->state  = EMPTY_SLOT;
  }

  // Day slot
  day_slot.loaded = false;

  // Root layer
  Layer *root_layer = window_get_root_layer(&window);

  // Time slots
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    TimeSlot *time_slot = &time_slots[i];
    layer_init(&time_slot->layer, frame_for_time_slot(time_slot));
    layer_add_child(root_layer, &time_slot->layer);
  }

  int date_container_height = SCREEN_HEIGHT - SCREEN_WIDTH;

  // Date container
  layer_init(&date_container_layer, GRect(0, SCREEN_WIDTH, SCREEN_WIDTH, date_container_height));
  layer_set_update_proc(&date_container_layer, &draw_date_container);
  layer_add_child(root_layer, &date_container_layer);

  // Day
  GRect day_layer_frame = GRect(
    MARGIN, 
    date_container_height - DAY_IMAGE_HEIGHT - MARGIN, 
    DAY_IMAGE_WIDTH, 
    DAY_IMAGE_HEIGHT
  );
  layer_init(&day_slot.layer, day_layer_frame);
  layer_add_child(&date_container_layer, &day_slot.layer);

  // Date
  GRect date_layer_frame = GRectZero;
  date_layer_frame.size.w   = DATE_IMAGE_WIDTH + MARGIN + DATE_IMAGE_WIDTH + DATE_PART_SPACE + DATE_IMAGE_WIDTH + MARGIN + DATE_IMAGE_WIDTH;
  date_layer_frame.size.h   = DATE_IMAGE_HEIGHT;
  date_layer_frame.origin.x = (SCREEN_WIDTH - date_layer_frame.size.w) / 2;
  date_layer_frame.origin.y = date_container_height - DATE_IMAGE_HEIGHT - MARGIN;

  layer_init(&date_layer, date_layer_frame);
  layer_add_child(&date_container_layer, &date_layer);

  // Year
  GRect year_layer_frame = GRect(
    SCREEN_WIDTH - YEAR_IMAGE_WIDTH - MARGIN - YEAR_IMAGE_WIDTH - MARGIN, 
    date_container_height - YEAR_IMAGE_HEIGHT - MARGIN, 
    YEAR_IMAGE_WIDTH + MARGIN + YEAR_IMAGE_WIDTH, 
    YEAR_IMAGE_HEIGHT
  );
  layer_init(&year_layer, year_layer_frame);
  layer_add_child(&date_container_layer, &year_layer);


  // Display
  PblTm tick_time;
  get_time(&tick_time);

  display_time(&tick_time);
  display_day(&tick_time);
  display_date(&tick_time);
  display_year(&tick_time);

}

void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *event) {

  if ((event->units_changed & DAY_UNIT) == DAY_UNIT) {
    display_day(event->tick_time);
    display_date(event->tick_time);
  }
  if ((event->units_changed & YEAR_UNIT) == YEAR_UNIT) {
    display_year(event->tick_time);
  }
  display_time(event->tick_time);
}

void handle_deinit(AppContextRef ctx) {
  for (int i = 0; i < NUMBER_OF_TIME_SLOTS; i++) {
    unload_digit_image_from_time_slot(&time_slots[i]);
  }
  for (int i = 0; i < NUMBER_OF_DATE_SLOTS; i++) {
    unload_digit_image_from_date_slot(&date_slots[i]);
  }
  for (int i = 0; i < NUMBER_OF_YEAR_SLOTS; i++) {
    bmp_deinit_container(&year_image_containers[i]);
  }

  if (day_slot.loaded) {
    bmp_deinit_container(&day_slot.image_container);
  }
}
