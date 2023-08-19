#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

AsyncWebServer server(80);
TaskHandle_t TaskLoopCore0;
TaskHandle_t TaskLoopCore1;

bool motorSwitchState = false;
bool brightnessSwitchState = false;
bool colourSwitchState = false;
bool stateSwitchState = false;

float brightness = 0.0;

#define MAX_WIFI_ATTEMPTS 10
#define WIFI_RETRY_DELAY 500
bool wifiConnected = false;

#pragma region Function Declarations
void LoopOutputHandle( void * pvParameters );
void LoopStateHandle( void * pvParameters );
void connectToWiFi();

void setRGBWLed(int red, int green, int blue, int white);
void handlePowerState();
void handleRGBWState();
void handleMotorState();
void handleBrightnessState();

void checkSwitch(int switchPin, bool &switchState, void (*callback)());
void handleStateSwitch();
void handleMotorSwitch();
void handleBrightnessSwitch();
void handleColourSwitch();

template <typename T> 
T incrementEnum(T &enumValue, T lastEnumValue);

template <typename EnumType>
void handleSwitch(EnumType &enumState, EnumType lastEnumValue, const char *switchName);
#pragma endregion

#pragma region Wifi Settings
const char* SSID = "ssid";
const char* PASSWORD = "pass";
const char* HOSTNAME = "GalaxyProjector-Dev";
const IPAddress STATIC_IP(192, 168, 0, 50);
const IPAddress GATEWAY(192, 168, 0, 1);
const IPAddress SUBNET(255, 255, 255, 0);
#pragma endregion

#pragma region Pin Definitions
#define RED_LED 17            // Green wire
#define WHITE_LED 18          // Blue wire
#define GREEN_LED 19          // White wire
#define BLUE_LED 21           // Red wire
#define PROJECTOR_LED 27      // Moon projector
#define MOTOR_BJT 4           // Motor control
#define MOTOR_SWITCH 32       // Motor switch
#define BRIGHTNESS_SWITCH 33  // Brightness switch
#define COLOUR_SWITCH 25      // Colour switch
#define STATE_SWITCH 26       // State switch
#pragma endregion

#pragma region State Definitions
enum PowerStateEnum {
  PowerOff,
  On,
  Project,
  PowerLast
};
PowerStateEnum pStates = PowerOff;

enum RGBWStateEnum {
  Blue,
  Red,
  Green,
  White,
  BlueRed,
  BlueGreen,
  RedGreen,
  RedWhite,
  GreenWhite,
  RedGreenBlue,
  BlueGreenWhite,
  BlueRedGreenWhite,
  Cycle,
  LedLast
};
RGBWStateEnum rgbwStates = Blue;

enum MotorStateEnum {
  MotorOff,
  Fast,
  Slow,
  MotorLast
};
MotorStateEnum mStates = MotorOff;

enum BrightnessStateEnum {
  ExtraLow,
  Low,
  Medium,
  High,
  BrightnessLast
};
BrightnessStateEnum bStates = ExtraLow;
#pragma endregion

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  #pragma region Pin Initialisation
  pinMode(RED_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(PROJECTOR_LED, OUTPUT);
  pinMode(MOTOR_BJT, OUTPUT);
  
  // Switches are active low so use INPUT_PULLUP
  pinMode(MOTOR_SWITCH, INPUT_PULLUP);
  pinMode(BRIGHTNESS_SWITCH, INPUT_PULLUP);
  pinMode(COLOUR_SWITCH, INPUT_PULLUP);
  pinMode(STATE_SWITCH, INPUT_PULLUP);
  #pragma endregion

  connectToWiFi();

  if(wifiConnected) {
    Serial.println("Initialising OTA");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Hi! This is a sample response.");
    });
    AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("OTA initialised");
  }
  else {
    Serial.println("WiFi not connected, OTA not initialised. Continuing offline...");
  }

  Serial.println("Initialising Tasks");
  
  Serial.print("Initialising TaskLoopCore1... ");
  xTaskCreatePinnedToCore(
    LoopOutputHandle,     /* Task function. */
    "TaskLoopCore1",      /* name of task. */
    10000,                /* Stack size of task */
    NULL,                 /* parameter of the task */
    1,                    /* priority of the task */
    &TaskLoopCore0,       /* Task handle to keep track of created task */
    1);                   /* pin task to core 1 */          
  delay(500); 

  Serial.print("Initialising TaskLoopCore0... ");
  xTaskCreatePinnedToCore(
    LoopStateHandle,            /* Task function. */
    "TaskLoopCore0",      /* name of task. */
    10000,                /* Stack size of task */
    NULL,                 /* parameter of the task */
    1,                    /* priority of the task */
    &TaskLoopCore1,       /* Task handle to keep track of created task */
    0);                   /* pin task to core 0 */
  delay(500); 
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  WiFi.setHostname(HOSTNAME);
  WiFi.config(STATIC_IP, GATEWAY, SUBNET);

  // Attempt to connect to WiFi
  int attemptCount = 0;
  while (WiFi.status() != WL_CONNECTED && attemptCount < MAX_WIFI_ATTEMPTS) {
    delay(WIFI_RETRY_DELAY);
    Serial.print(".");
    attemptCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
  } else {
    Serial.println("\nFailed to connect to WiFi");
    // Continue offline
    return;
  }
}

void loop() {

}

#pragma region Output Handlers

void LoopOutputHandle( void * pvParameters ){
  Serial.print("TaskLoopCore1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    handlePowerState();
    if (pStates == PowerStateEnum::PowerOff) continue;
    handleBrightnessState();
    handleRGBWState();
    handleMotorState();
  }
}

void handlePowerState() {
  switch(pStates) {
    case PowerStateEnum::PowerOff:
      analogWrite(RED_LED, 0);
      analogWrite(GREEN_LED, 0);
      analogWrite(BLUE_LED, 0);
      analogWrite(WHITE_LED, 0);
      digitalWrite(PROJECTOR_LED, LOW);
      analogWrite(MOTOR_BJT, 0);
      break;
    case PowerStateEnum::On:
      
      break;
    case PowerStateEnum::Project:
      digitalWrite(PROJECTOR_LED, HIGH);
      break;
    default:
      Serial.println("Invalid Power State");
      break;
  }
}

void handleBrightnessState() {
  switch(bStates) {
    case BrightnessStateEnum::ExtraLow:
      brightness = 0.25;
      break;
    case BrightnessStateEnum::Low:
      brightness = 0.5;
      break;
    case BrightnessStateEnum::Medium:
      brightness = 0.75;
      break;
    case BrightnessStateEnum::High:
      brightness = 1;
      break;
    default:
      Serial.println("Invalid Brightness State");
      break;
  }
}

// Set the RGBW LED to the appropriate colour multiplied by the brightness
void handleRGBWState() {
  switch(rgbwStates) {
    case RGBWStateEnum::Blue:
      setRGBWLed(0, 0, 255, 0);
      break;
    case RGBWStateEnum::Red:
      setRGBWLed(255, 0, 0, 0);
      break;
    case RGBWStateEnum::Green:
      setRGBWLed(0, 255, 0, 0);
      break;
    case RGBWStateEnum::White:
      setRGBWLed(0, 0, 0, 255);
      break;
    case RGBWStateEnum::BlueRed:
      setRGBWLed(255, 0, 255, 0);
      break;
    case RGBWStateEnum::BlueGreen:
      setRGBWLed(0, 255, 255, 0);
      break;
    case RGBWStateEnum::RedGreen:
      setRGBWLed(255, 255, 0, 0);
      break;
    case RGBWStateEnum::RedWhite:
      setRGBWLed(255, 0, 0, 255);
      break;
    case RGBWStateEnum::GreenWhite:
      setRGBWLed(0, 255, 0, 255);
      break;
    case RGBWStateEnum::RedGreenBlue:
      setRGBWLed(255, 255, 255, 0);
      break;
    case RGBWStateEnum::BlueGreenWhite:
      setRGBWLed(0, 255, 255, 255);
      break;
    case RGBWStateEnum::BlueRedGreenWhite:
      setRGBWLed(255, 255, 255, 255);
      break;
    case RGBWStateEnum::Cycle:
      // Cycle through the colours using a sine wave on millis()
      setRGBWLed(
        127.5 * (1 + sin(millis() / 1000.0)),               // Red
        127.5 * (1 + sin(millis() / 1000.0 + 2 * PI / 3)),  // Green
        127.5 * (1 + sin(millis() / 1000.0 + 4 * PI / 3)),  // Blue
        0);                                                 // White
      break;
    default:
      Serial.println("Invalid RGBW State");
      break;
  }
}

void handleMotorState() {
  switch(mStates) {
    case MotorStateEnum::MotorOff:
      analogWrite(MOTOR_BJT, 0);
      break;
    case MotorStateEnum::Fast:
      analogWrite(MOTOR_BJT, 255);
      break;
    case MotorStateEnum::Slow:
      analogWrite(MOTOR_BJT, 200);
      break;
    default:
      Serial.println("Invalid Motor State");
      break;
  }
}

void setRGBWLed(int red, int green, int blue, int white) {
  analogWrite(RED_LED, red * brightness);
  analogWrite(GREEN_LED, green * brightness);
  analogWrite(BLUE_LED, blue * brightness);
  analogWrite(WHITE_LED, white * brightness);
}
#pragma endregion

#pragma region State Handlers
/* 
 * This task is pinned to core 1
 * It is used to monitor the state of the switches
*/
void LoopStateHandle( void * pvParameters ){
  Serial.print("TaskLoopCore0 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    checkSwitch(MOTOR_SWITCH, motorSwitchState, handleMotorSwitch);
    checkSwitch(BRIGHTNESS_SWITCH, brightnessSwitchState, handleBrightnessSwitch);
    checkSwitch(COLOUR_SWITCH, colourSwitchState, handleColourSwitch);
    checkSwitch(STATE_SWITCH, stateSwitchState, handleStateSwitch);
    
    // Delay for 10ms to prevent the task from hogging the CPU
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * Checks the state of a switch connected to the specified pin and updates the switch state accordingly.
 * 
 * @param switchPin The digital pin to which the switch is connected.
 * @param switchState A reference to a boolean variable representing the current state of the switch.
 *                    This variable will be updated based on the switch's state.
 * @param callback A callback function that will be called when the switch is pressed.
 *                 The callback function should have the signature: void functionName()
 */
void checkSwitch(int switchPin, bool &switchState, void (*callback)()) {
  static unsigned long lastDebounceTime = 0;
  static unsigned long timeReleased = 0;    // Using static variables to preserve state between function calls
  const unsigned long debounceDelay = 100;   // Debounce delay in milliseconds
  unsigned long currentMillis = millis();

  int switchReading = digitalRead(switchPin);

  if (switchReading == LOW) { // If the switch is pressed
    if (!switchState) {
      switchState = true;
      callback(); // Call the callback function
    }
  } // No time debounce is required for the switch being pressed as the change in state provides this functionality
  
  else { // If the switch is released
    // Calculate the time since the switch was released
    unsigned long timeSinceRelease = currentMillis - timeReleased;
    if(switchState && timeSinceRelease > debounceDelay) {
      switchState = false;
      timeReleased = currentMillis;
    }
  } // A debounce delay is required for the switch being released as the change in state does not prevent the switch from activating immediately after being released
}

void handleStateSwitch() {
  handleSwitch(pStates, PowerStateEnum::PowerLast, "Power");
}

void handleMotorSwitch() {
  handleSwitch(mStates, MotorStateEnum::MotorLast, "Motor");
}

void handleBrightnessSwitch() {
  handleSwitch(bStates, BrightnessStateEnum::BrightnessLast, "Brightness");
}

void handleColourSwitch() {
  handleSwitch(rgbwStates, RGBWStateEnum::LedLast, "Colour");
}

template <typename EnumType>
void handleSwitch(EnumType &enumState, EnumType lastEnumValue, const char *switchName) {
  Serial.print(switchName);
  Serial.print(" Switch Pressed - ");
  incrementEnum(enumState, lastEnumValue);
  Serial.println(static_cast<int>(enumState));
}

/**
 * Increments an enumeration value and wraps it around based on the provided range.
 *
 * This function increments the given enumeration value and ensures it wraps around
 * within the range defined by the last enumeration value. The incrementing and wrapping
 * behavior is calculated as (enumValue + 1) % lastEnumValue.
 *
 * @param enumValue A reference to the enumeration value to be incremented.
 * @param lastEnumValue The last enumeration value in the range, defining the wrapping point.
 * @return The incremented enumeration value after wrapping.
 */
template <typename T>
T incrementEnum(T &enumValue, T lastEnumValue) {
  enumValue = static_cast<T>((enumValue + 1) % lastEnumValue);
  return enumValue;
}
#pragma endregion