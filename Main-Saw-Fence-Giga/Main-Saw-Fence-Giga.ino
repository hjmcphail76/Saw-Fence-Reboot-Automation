#include <Arduino_GigaDisplayTouch.h>
#include <Arduino_H7_Video.h>
#include <lvgl.h>
#include <ui.h>

/* Initialize the GIGA Display Shield with a resolution of 800x480 pixels */
Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch Touch;

int label_val;

static void ButtonMeasureEventHandler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
  }
}

void setup() {
  Display.begin();
  Touch.begin();

  /* Initialize the user interface designed with SquareLine Studio */
  ui_init();

  /* Add buttons event handlers */
  lv_obj_add_event_cb(ui_MeasureButton, ButtonMeasureEventHandler, LV_EVENT_ALL, NULL);

  /* Set initial value of the label to zero */
  label_val = 0;
  lv_label_set_text_fmt(ui_CurrentMeasurementLabel, "%d", label_val);

  lv_scr_load(ui_SplashScreen); //Load the splash screen until the clearcore sends the OK over serial.
}

void loop() {
  /* Feed LVGL engine */
  lv_timer_handler();
}