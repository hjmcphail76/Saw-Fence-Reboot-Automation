#include <ClearCore.h>
#include <genieArduinoDEV.h>
#include "MechanismClasses.h"
#include "MotorClasses.h"

/*
Neo7CNC Automated Chop Saw fence

Hardware used:
  -Teknic ClearCore controller

  -Arduino Giga+Giga Shield Screen  OR  4D Systems 'Gen4 uLCD 43D CLB' Screen with the 'Clearcore to 4D' adapter.

  -Clearpath MC or SD servo
    -Both are supported
    -Plug MC type servo into port 0 on the Clearcore.
    -Plug SD type servo into ports 3 on the Clearcore.
    -No code changes are required. Everything other than what is listed in 'User Configuration' is taken care on the backend for you.
*/


//--------------------------------------------------User Configuration start: -------------------------------------------------------------------------------


//Default units for the system to boot with. UnitType::UNIT_INCHES  or  UnitType::UNIT_MILLIMETERS
UnitType currentUnits = UnitType::UNIT_INCHES;
// currentUnits = UnitType::UNIT_MILLIMETERS;


// IMPORTANT: Uncomment ONLY ONE of the 'Mechanism* currentMechanism = new ...' lines below
// to select the mechanism you are currently configuring. Fill in the parameters (motorProgInputRes, maxAccel, maxVel, mechanism_specific_parameter, UnitType)
// The unit type here is just for configuration of your system. Swapping between units for cut measurements on the fly can be done on the system in settings
// and will not effect this.

// Example: Belt Mechanism with 1.0 inch pulley diameter and a 1:1 motor gearbox reduction (none)
Mechanism* currentMechanism = new BeltMechanism(200, 5000, 500, 0.236, 1.0, UNIT_INCHES);

// Example: Leadscrew Mechanism with 0.2 inches per revolution pitch and a 1:1 motor gearbox reduction (none)
// Mechanism* currentMechanism = new LeadscrewMechanism(2000, 5000, 500, 0.2, 1, UNIT_INCHES);

// Example: Rack and Pinion Mechanism with 0.5 inch pinion diameter and a 1:1 motor gearbox reduction (none)
// Mechanism* currentMechanism = new RackAndPinionMechanism(100, 15000, 3000, 0.5, 1, UNIT_INCHES);

Value maxTravel = Value(47.0, UnitType::UNIT_INCHES);  //This 'Value' structure is defined in mechanismClasses.h and holds our value + whatever units

Value homeToBladeOffset = Value(35, UnitType::UNIT_INCHES);  //Before changing this value, leave it default and deploy everything. Then when running, home the system and measure from blade to saw stop at home position.


//Serial Monitor Settings:
const int serialMoniterBaudRate = 115200;

//4D UART settings:
const int genieBaudRate = 9600;

//Arduino Giga UART settings:
const int gigaBaudRate = 9600;


//--------------------------------------------------User Configuration end: --------------------------------------------------------------------------------


Genie genie;
Motor* motor;

// -------------------------------------------------------------------------

// Declare our user-defined helper functions: ------------------------------
void SetMeasurementUIDisplay();
bool ParameterInput(genieFrame Event);
String GetParameterInputValue();
void myGenieEventHandler(void);
String getUnitString(UnitType unit);
double convertFromInches(float valueInInches, UnitType targetUnit);
double convertToInches(double value, UnitType unit);
double convertUnits(double value, UnitType from, UnitType to);
void OpenParameterOutsideRangeError();
float GetParameterEnteredAsFloat();

//---------------------------------------------------

//Misc:
char keyvalue[10];                   // Array to hold keyboard character values
int counter = 0;                     // Keyboard number of characters
int temp, sumTemp;                   // Keyboard Temp values (sumTemp is not actively used for current value display)
int newValue;                        // Not actively used for current value display
float currentMainMeasurement = 0.0;  // The target measurement value entered by the user
int displayMsTime = 1250;            //Time errors and alert screens show in milliseconds.

//Keep track of the state of what parameter the user is editing and what should the keyboard edit.
enum InputMode {
  INPUT_MEASUREMENT,
  INPUT_HOME_TO_BLADE_OFFSET
  // Add more as needed
};
InputMode currentInputMode = INPUT_MEASUREMENT;  //default

//----------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(serialMoniterBaudRate);
  delay(5000);  // Give time for serial monitor to connect
  Serial.println("Serial Monitor init");

  // Initialize Serial1 for the 4D Display
  ConnectorCOM1.RtsMode(SerialBase::LINE_OFF);
  Serial1.begin(genieBaudRate);  // Use genieBaudRate for consistency
  Serial1.ttl(true);

  // Initialize Serial0 for the Giga display
  ConnectorCOM0.RtsMode(SerialBase::LINE_OFF);
  Serial0.begin(gigaBaudRate);
  Serial0.ttl(true);

  delay(3000);

  motor = new SDMotor(currentMechanism);
  //motor = new MCMotor(currentMechanism);

  motor->HandleAlerts();  // Handle any initial motor alerts


  Serial.println("Attempting to initialize Genie...");
  // Loop until Genie display is online
  while (!genie.Begin(Serial1)) {
    Serial.println("waiting for Genie...");
    delay(250);
  }

  if (genie.IsOnline()) {
    Serial.println("Genie initialization successful. Display is online!");
    genie.AttachEventHandler(myGenieEventHandler);  // Attach the event handler
    genie.SetForm(1);                               // Set the initial form to 1
    Serial.println(genie.GetForm());
  } else {
    Serial.println("Genie initialization failed.");
  }
}

void loop() {
  genie.DoEvents();  // Keep the Genie library processing

  // Continuously call motor homing state machine updates
  motor->StateMachinePeriodic(genie);
}

// Genie event handler function
void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event);


  // Handle events based on object type and index
  if (Event.reportObject.object == GENIE_OBJ_WINBUTTON) {
    Serial.println(Event.reportObject.index);

    switch (Event.reportObject.index) {
      case 0:  // Measure button
        Serial.println("Measure pressed");

        if (motor->hasHomed) {

          double position = convertToInches(homeToBladeOffset.val, homeToBladeOffset.unit) - convertToInches(currentMainMeasurement, currentUnits);
          if (position > 0) {
            // Convert the position to motor steps
            motor->MoveAbsolutePosition(static_cast<int32_t>(position * currentMechanism->CalculateStepsPerUnit()));
          } else {
          }
        } else {
          genie.SetForm(6);
          delay(displayMsTime);
          genie.SetForm(1);
        }
        break;
      case 1:  // Edit Measurement button
        Serial.println("Edit Measurement pressed");
        currentInputMode = InputMode::INPUT_MEASUREMENT;
        genie.SetForm(2);  // Switch to the keyboard input form
        break;
      case 2:  // HOME BUTTON
        Serial.println("HOME BUTTON PRESSED");
        genie.SetForm(5);  // Show homing in progress screen
        motor->StartSensorlessHoming();
        break;
      case 3:  // Reset Servo button
        Serial.println("Reset Servo pressed");
        motor->hasHomed = false;  //require re homing
        motor->HandleAlerts();
        break;
      case 4:  // Settings Button
        Serial.println("Settings Button Pressed");
        genie.SetForm(3);  // Switch to the settings form
        break;
      case 5:  //Edit home to blade offset
        Serial.println("Edit home to blade offset Button Pressed");
        genie.SetForm(2);  // Switch to the keyboard input form
        currentInputMode = InputMode::INPUT_HOME_TO_BLADE_OFFSET;
        break;
    }
  }
  // Handle keyboard input events. Based on the current parameter being entered, that value gets changed/updated
  if (Event.reportObject.object == GENIE_OBJ_KEYBOARD) {

    if (ParameterInput(Event)) {
      //updates to happen when user is done are here:

      switch (currentInputMode) {
        case INPUT_MEASUREMENT:
          SetMeasurementUIDisplay();  // Update the main screen label with the new value
          break;
        case INPUT_HOME_TO_BLADE_OFFSET:
          homeToBladeOffset.val = GetParameterEnteredAsFloat();
          //Serial.println("Current INPUT_HOME_TO_BLADE_OFFSET: " + String(homeToBladeOffset));
          genie.SetForm(3);
          break;
      }
    }

    //Updates to happen each key entered here. This is the label on the keyboard form, and is universal for all entered parameters.
    genie.WriteInhLabel(1, String(keyvalue));
  }
}

//UI Functions: --------------------------------------------


void SetMeasurementUIDisplay() {
  genie.SetForm(1);  // Go back to the main form

  String paramValue = GetParameterInputValue();  // Only call this ONCE
  String combinedString = paramValue + getUnitString(currentUnits);
  genie.WriteInhLabel(0, combinedString.c_str());

  float val = paramValue.toFloat();  // Parse string to float once

  // Optional: Check if input was valid
  if (paramValue.length() == 0 || isnan(val)) {
    OpenParameterOutsideRangeError();  // Custom error for invalid float
    return;
  }

  double maxTravelInCurrentUnits = convertUnits(maxTravel.val, maxTravel.unit, currentUnits);

  if (val < maxTravelInCurrentUnits) {
    // No unit conversion needed if val is already in currentUnits
    currentMainMeasurement = val;
    Serial.println(currentMainMeasurement);
  } else {
    OpenParameterOutsideRangeError();  // Handle out-of-range input
  }
}


/*Returns 0 if an error occurs with the operation.*/
float GetParameterEnteredAsFloat() {
  String rawString = GetParameterInputValue();
  float val = rawString.toFloat();

  // Basic check for invalid conversion
  if (val == 0.0 && !rawString.startsWith("0")) {
    Serial.println("Error converting input to float: ");
    Serial.println(rawString);
    return 0.0;
  } else {
    return val;
  }
}


/*
Returns true when user presses Enter. Updates the global 'currentMainMeasurement'.
Some of this code is adapted from the ClearCore/HMI example project.
*/
bool ParameterInput(genieFrame Event) {
  // Get the value of the key pressed
  temp = genie.GetEventData(&Event);

  // Check if the key is a digit (0-9) and if we have space in the array
  if (temp >= 48 && temp <= 57 && counter < 9) {
    keyvalue[counter] = temp;      // Append the decimal value of the key pressed
    keyvalue[counter + 1] = '\0';  // Null-terminate the string
    counter++;
  } else if (temp == 110 && counter < 9) {  // '.' pressed (ASCII for '.')
    // Allow only one decimal point
    bool hasDecimal = false;
    for (int i = 0; i < counter; i++) {
      if (keyvalue[i] == '.') {
        hasDecimal = true;
        break;
      }
    }
    if (!hasDecimal && counter < 9) {
      keyvalue[counter] = '.';       // Append the decimal character
      keyvalue[counter + 1] = '\0';  // Null-terminate the string
      counter++;
    }
  } else if (temp == 8) {  // Check if Backspace/delete Key (ASCII for Backspace)
    if (counter > 0) {
      counter--;              // Decrement the counter
      keyvalue[counter] = 0;  // Overwrite the last position with null
                              // TODO: Update the LED_DIGITS object on the display if you have one
                              // genie.WriteObject(GENIE_OBJ_LED_DIGITS, 18, atoi(keyvalue)); // Update display
    }
  } else if (temp == 13) {  // Check if 'Enter' Key (ASCII for Enter)
    return true;            // Indicate that Enter was pressed
  }
  return false;  // Indicate that Enter was not pressed
}

/*
We have a function to get the value as a String object, then discard keyvalue's contents for the next time it gets used.
*/
String GetParameterInputValue() {
  String stringKeyValue = String(keyvalue);
  for (int f = 0; f < 10; f++) {
    keyvalue[f] = 0;
  }
  counter = 0;
  return stringKeyValue;
}

void OpenParameterOutsideRangeError() {
  genie.SetForm(4);
  delay(displayMsTime);
  genie.SetForm(2);
}

// #include "ClearCore.h"

// void setup() {
//   Serial0.begin(9600);  // COM0 UART at 9600 baud
//   Serial.begin(115200); // USB Serial for debug

//   Serial.println("ClearCore Serial0 started");
// }

// void loop() {
//   Serial0.println("Hello from ClearCore COM0");
//   Serial.println("Sent message on Serial0");

//   delay(1000);
// }
