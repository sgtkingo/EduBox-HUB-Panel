
/**
 * @file splasher.h
 * @brief Modal popup utility for LVGL with OK/Cancel buttons and optional auto-close.
 *
 * Provides a function to display a modal popup (message box) with OK and Cancel buttons,
 * and an optional auto-close timer. Intended for use with the LVGL graphics library.
 *
 * @copyright 2025 MTA
 * @author Ing. Jiri Konecny
 */

#include "splasher.hpp"
#include "logs.hpp"

#if defined(ARDUINO_H_ENV) && defined(ARDUINO)
    #include <Arduino.h>  ///< Include Arduino Serial functions
#elif defined(STDIO_H_ENV)
    #include <stdio.h>    ///< Include standard I/O functions
#endif

#ifdef USE_LVGL

struct splash_data_t {
  lv_obj_t* mbox;
  lv_timer_t* timer;
};

static void on_splash_msgbox_event(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  
  if (code == LV_EVENT_VALUE_CHANGED) {
    splash_data_t* data = (splash_data_t*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
    if (data && data->mbox)
    {
      lv_msgbox_close(data->mbox);
    }
  }
  else if (code == LV_EVENT_DELETE) {
    // Clean up when the message box is being deleted
    splash_data_t* data = (splash_data_t*)lv_obj_get_user_data((lv_obj_t*)lv_event_get_target(e));
    
    if (data) {
      // Cancel the timer if it exists
      if (data->timer) {
        lv_timer_del(data->timer);
        data->timer = NULL;
      }
      
      // Clean up the data structure
      delete data;
    }
  }
}

void show_splash_popup(const char* title, const char* text, uint32_t autoclose_ms) {
  static const char* btns[] = {""};
  lv_obj_t* scr = lv_scr_act();
  lv_obj_t* mbox = lv_msgbox_create(scr, title, text, btns, true);
  lv_obj_center(mbox);
  
  // Create data structure to track both mbox and timer
  splash_data_t* data = new splash_data_t{mbox, NULL};
  lv_obj_set_user_data(mbox, data);
  
  lv_obj_add_event_cb(mbox, on_splash_msgbox_event, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(mbox, on_splash_msgbox_event, LV_EVENT_DELETE, NULL);

  if (autoclose_ms > 0) {
    lv_timer_t* t = lv_timer_create([](lv_timer_t* t){
      splash_data_t* data = (splash_data_t*)t->user_data;
      if (data && data->mbox) {
        // Clear the timer reference before closing
        data->timer = NULL;
        lv_msgbox_close(data->mbox);
        // Don't delete data here - it will be deleted in LV_EVENT_DELETE handler
      }
      lv_timer_del(t);
    }, autoclose_ms, data);
    data->timer = t;
  }
}

#else
// LVGL not enabled, provide empty implementations, with logMessage instead
static void on_splash_msgbox_event(void* e) 
{
  // No operation
}

void show_splash_popup(const char* title, const char* text, uint32_t autoclose_ms) {
  debugLogMessage(DEBUG_VERBOSE_IMPORTANT, "splashMessage", "popup", "%s - %s", title, text);
}

#endif // USE_LVGL

void delay_ms(unsigned int ms) {
    #if defined(ARDUINO_H_ENV) && defined(ARDUINO)
        delay(ms); // Arduino delay
    #elif defined(_WIN32) || defined(_WIN64)
        Sleep(ms); // Windows sleep
    #else
        usleep(ms * 1000); // POSIX sleep
    #endif
}

void splashMessage(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);

    #ifdef SPLASHER_H
     show_splash_popup("Message", buffer, SPLASHER_TIMEOUT_MS); // Show splash for 5 seconds
    #endif   

    va_end(args);
}

