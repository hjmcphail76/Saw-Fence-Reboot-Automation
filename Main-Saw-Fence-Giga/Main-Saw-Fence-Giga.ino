#include <Arduino_GigaDisplayTouch.h>
#include <Arduino_H7_Video.h>
#include <lvgl.h>
#include <ui.h>

/* Initialize the GIGA Display Shield at 800×480 */
Arduino_H7_Video Display(800, 480, GigaDisplayShield);
Arduino_GigaDisplayTouch Touch;

bool isConnected = false;
lv_obj_t* active_text_area = nullptr;
static String lastText = "";

/* --- Main button handler --- */
static void ButtonEventHandler(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    lv_obj_t* btn = lv_event_get_target(e);
    Serial.println("btn pressed");

    if (btn == ui_MEASURE_BUTTON) Serial2.println("BUTTON:2");
    else if (btn == ui_EDIT_TARGET_BUTTON) Serial2.println("BUTTON:3");
    else if (btn == ui_HOME_AXIS_BUTTON) Serial2.println("BUTTON:4");
    else if (btn == ui_RESET_SERVO_BUTTON) Serial2.println("BUTTON:5");
    else if (btn == ui_SETTINGS_BUTTON) Serial2.println("BUTTON:6");
    else if (btn == ui_EDIT_MAX_TRAVEL_BUTTON) Serial2.println("BUTTON:7");
    else if (btn == ui_EXIT_SETTINGS_BUTTON) Serial2.println("BUTTON:10");
    else Serial.println("Unknown button clicked");
  }

  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t* obj = lv_event_get_target(e);

    if (obj == ui_UNIT_SWITCH) {
      bool isChecked = lv_obj_has_state(obj, LV_STATE_CHECKED);
      Serial.print("UNIT_SWITCH state: ");
      Serial.println(isChecked ? "ON" : "OFF");

      Serial2.print("BUTTON:");
      Serial2.println(isChecked ? "12" : "11");
    }
  }
}

/* --- TextArea handler to catch digits, backspace, ENTER --- */
static void TextAreaEventHandler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  String currentText = String(lv_textarea_get_text(ui_PARAMETER_INPUT_TEXT_AREA));

  if (code == LV_EVENT_INSERT) {
    const char* txt = (const char*)lv_event_get_param(e);
    if (txt && *txt) {
      if (String(txt) == ".") {
        if (currentText.indexOf('.') != -1) {
          String text = currentText;
          if (text.length() > 0) {
            text.remove(text.length() - 1);  // Remove last character ('.')
            lv_textarea_set_text(ui_PARAMETER_INPUT_TEXT_AREA, text.c_str());
          }
          return;
        }
      }

      Serial2.print("KEY:");
      Serial2.println(txt);
    }
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    Serial.print("newtxt: ");
    Serial.println(currentText);

    Serial.print("lasttxt: ");
    Serial.println(currentText);
    if (currentText.length() < lastText.length()) {
      Serial.println("Backspace detected!");
      Serial2.println("KEY:BACKSPACE");
      return;
    }

    if (currentText.indexOf("ENTER") != -1) {
      Serial.println("Enter has been pressed...");
      Serial2.println("KEY:ENTER");
      lastText = "";
      return;
    }

    lastText = currentText;
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
  Serial2.begin(9600);

  Serial.println("Init done.");

  // wait for handshake
  while (!isConnected) {
    lv_timer_handler();
    if (Serial2.available()) {
      String m = Serial2.readStringUntil('\n');
      Serial.println("Received: " + m);
      m.trim();
      if (m == "HELLO") {
        Serial2.println("ACK");
        Serial2.flush();
        Serial.println("Sent ACK to clearcore");
        isConnected = true;
      }
    }
  }

  // Button bindings
  lv_obj_add_event_cb((lv_obj_t*)ui_MEASURE_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_EDIT_TARGET_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_HOME_AXIS_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_RESET_SERVO_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_SETTINGS_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_EDIT_MAX_TRAVEL_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_EXIT_SETTINGS_BUTTON, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb((lv_obj_t*)ui_UNIT_SWITCH, ButtonEventHandler, LV_EVENT_VALUE_CHANGED, nullptr);

  // Textarea event bindings
  lv_obj_add_event_cb(ui_PARAMETER_INPUT_TEXT_AREA, TextAreaEventHandler, LV_EVENT_INSERT, nullptr);
  lv_obj_add_event_cb(ui_PARAMETER_INPUT_TEXT_AREA, TextAreaEventHandler, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui_PARAMETER_INPUT_TEXT_AREA, TextAreaEventHandler, LV_EVENT_READY, nullptr);
}

void loop() {
  lv_timer_handler();

  if (Serial2.available()) {
    String msg = Serial2.readStringUntil('\n');
    Serial.println(msg);
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
          lv_keyboard_set_textarea(ui_PARAMETER_INPUT_KEYBOARD, ui_PARAMETER_INPUT_TEXT_AREA);
          lv_obj_add_state(ui_PARAMETER_INPUT_TEXT_AREA, LV_STATE_FOCUSED);
          lv_textarea_set_cursor_pos(ui_PARAMETER_INPUT_TEXT_AREA, LV_TEXTAREA_CURSOR_LAST);
          break;
        case 3:
          lv_scr_load(ui_SETTINGS_SCREEN);
          break;
        case 4:
          lv_scr_load(ui_OUTSIDE_RANGE_ERROR_SCREEN);
          break;
        case 5:
          lv_scr_load(ui_HOMING_ALERT_SCREEN);
          break;
        case 6:
          lv_scr_load(ui_PLEASE_HOME_ERROR_SCREEN);
          break;
        default:
          lv_scr_load(ui_SPLASH_SCREEN);
          break;
      }
    } else if (msg.startsWith("SETLABEL:")) {
      Serial.print("Incoming setlabel: ");
      Serial.println(msg);
      int a = msg.indexOf(':'), b = msg.indexOf(':', a + 1);
      if (a >= 0 && b >= 0) {
        int li = msg.substring(a + 1, b).toInt();
        String labelText = msg.substring(b + 1);
        if (li == 1) {
          lv_scr_load(ui_MAIN_CONTROL_SCREEN);
          lv_label_set_text(ui_CURRENT_MEASUREMENT_LABEL, labelText.c_str());
          Serial.println(lv_label_get_text(ui_CURRENT_MEASUREMENT_LABEL));
        }
      }
    } else if (msg.startsWith("SETUNITSWITCH:")) {
      int a = msg.indexOf(':');
      // TODO: handle switch change
    }
  }

  delay(10);
}
