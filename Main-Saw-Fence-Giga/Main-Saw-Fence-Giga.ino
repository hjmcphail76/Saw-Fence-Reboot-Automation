#include <Arduino_GigaDisplayTouch.h>
#include <Arduino_H7_Video.h>
#include <lvgl.h>
#include <ui.h>

/* Initialize the GIGA Display Shield at 800×480 */
Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch Touch;

bool isConnected = false;
lv_obj_t* active_text_area = nullptr;

/* --- Main button handler --- */
static void ButtonEventHandler(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  lv_obj_t* btn = lv_event_get_target(e);
  Serial.println("btn pressed");

  if (btn == ui_MEASURE_BUTTON) Serial1.println("BUTTON:2");
  else if (btn == ui_EDIT_TARGET_BUTTON) Serial1.println("BUTTON:3");
  else if (btn == ui_HOME_AXIS_BUTTON) Serial1.println("BUTTON:4");
  else if (btn == ui_RESET_SERVO_BUTTON) Serial1.println("BUTTON:5");
  else if (btn == ui_SETTINGS_BUTTON) Serial1.println("BUTTON:6");
  else if (btn == ui_EDIT_HOME_TO_BLADE_OFFSET_BUTTON) Serial1.println("BUTTON:7");
  else Serial.println("Unknown button clicked");
}

/* --- TextArea handler to catch digits, backspace, ENTER --- */
static void TextAreaEventHandler(lv_event_t* e) {
  Serial.println("haha");
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_INSERT) {
    const char* txt = (const char*)lv_event_get_param(e);
    if (txt && *txt) {
      Serial1.print("KEY:");
      Serial1.println(txt);
    }
  } else if (code == LV_EVENT_DELETE) {
    Serial1.println("KEY:BACKSPACE");
  } else if (code == LV_EVENT_READY) {
    Serial1.println("KEY:ENTER");
  }
}

/* --- Numeric keyboard setup (unchanged) --- */
void setupNumericKeyboard(lv_obj_t* keyboard, lv_obj_t* ta = nullptr) {
  if (!keyboard) {
    Serial.println("Error: Keyboard is NULL");
    return;
  }
  active_text_area = ta;

  lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_4);

  static const char* kb_map[] = {
    "7", "8", "9", "\n",
    "4", "5", "6", "\n",
    "1", "2", "3", "\n",
    ".", "0", LV_SYMBOL_BACKSPACE, "ENTER", NULL
  };
  static const lv_btnmatrix_ctrl_t kb_ctrl[] = {
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1,
    2
  };

  lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_4, kb_map, kb_ctrl);
}

void setup() {
  Display.begin();
  Touch.begin();
  ui_init();

  lv_scr_load(ui_SPLASH_SCREEN);
  lv_timer_handler();

  Serial.begin(115200);
  Serial1.begin(9600);

  // wait for handshake
  while (!isConnected) {
    lv_timer_handler();
    if (Serial1.available()) {
      String m = Serial1.readStringUntil('\n');
      m.trim();
      if (m == "HELLO") {
        Serial1.println("ACK");
        isConnected = true;
      }
    }
  }


  lv_obj_add_event_cb((lv_obj_t*)ui_MEASURE_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_EDIT_TARGET_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_HOME_AXIS_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_RESET_SERVO_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_SETTINGS_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_EDIT_HOME_TO_BLADE_OFFSET_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);

  lv_obj_add_event_cb(ui_PARAMETER_INPUT_TEXT_AREA, TextAreaEventHandler, LV_EVENT_INSERT, nullptr);
  lv_obj_add_event_cb(ui_PARAMETER_INPUT_TEXT_AREA, TextAreaEventHandler, LV_EVENT_DELETE, nullptr);
  lv_obj_add_event_cb(ui_PARAMETER_INPUT_TEXT_AREA, TextAreaEventHandler, LV_EVENT_READY, nullptr);
}

void loop() {
  lv_timer_handler();

  if (Serial1.available()) {
    String msg = Serial1.readStringUntil('\n');
    msg.trim();
    if (msg.startsWith("SETSCREEN:")) {
      int idx = msg.substring(10).toInt();
      switch (idx) {
        case 0:
          lv_scr_load(ui_SPLASH_SCREEN);
          break;
        case 1:
          lv_scr_load(ui_MAIN_CONTROL_SCREEN);
          break;
        case 2:
          lv_scr_load(ui_PARAMETER_EDIT_SCREEN);
          lv_textarea_set_text(ui_PARAMETER_INPUT_TEXT_AREA, "");
          setupNumericKeyboard(ui_PARAMETER_INPUT_KEYBOARD, ui_PARAMETER_INPUT_TEXT_AREA);
          lv_keyboard_set_textarea(ui_PARAMETER_INPUT_KEYBOARD, ui_PARAMETER_INPUT_TEXT_AREA);  // <-- ADD THIS
          lv_obj_add_state(ui_PARAMETER_INPUT_TEXT_AREA, LV_STATE_FOCUSED);
          lv_textarea_set_cursor_pos(ui_PARAMETER_INPUT_TEXT_AREA, LV_TEXTAREA_CURSOR_LAST);
          break;

        case 3:
          lv_scr_load(ui_SETTINGS_SCREEN);
          break;
        default:
          lv_scr_load(ui_SPLASH_SCREEN);
          break;
      }
    } else if (msg.startsWith("SETLABEL:")) {
      Serial.println("incoming setlabel");
      int a = msg.indexOf(':'), b = msg.indexOf(':', a + 1);
      if (a >= 0 && b >= 0) {
        int li = msg.substring(a + 1, b).toInt();
        const char* txt = msg.substring(b + 1).c_str();
        if (li == 1) lv_label_set_text(ui_CURRENT_MEASUREMENT_LABEL, txt);
        if (li == 8) lv_textarea_set_text(ui_PARAMETER_INPUT_TEXT_AREA, txt);
      }
    }
  }


  delay(10);
}
