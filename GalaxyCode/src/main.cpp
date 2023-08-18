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

#pragma region Function Declarations
void LoopCore0( void * pvParameters );
void LoopCore1( void * pvParameters );
void checkSwitch(int switchPin, bool &switchState, void (*callback)());
#pragma endregion

#pragma region Wifi Settings
const char* SSID = "ssid";
const char* PASSWORD = "pass";
const char* HOSTNAME = "GalaxyProjector-Dev";
const IPAddress IP(192, 168, 0, 50);
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
enum LEDState {
  Off,
  Green,
  Blue,
  White,
  Brown,
  Motor,
  Projector
};
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

  #pragma region Wifi and OTA Setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  WiFi.setHostname(HOSTNAME);
  WiFi.config(IP, GATEWAY, SUBNET);

  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is a sample response.");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  server.begin();
  Serial.println("HTTP server started");
  #pragma endregion

  Serial.println("Initialising Tasks");

  Serial.print("Initialising TaskLoopCore0... ");
  xTaskCreatePinnedToCore(
    LoopCore0,          /* Task function. */
    "TaskLoopCore0",    /* name of task. */
    10000,              /* Stack size of task */
    NULL,               /* parameter of the task */
    1,                  /* priority of the task */
    &TaskLoopCore0,     /* Task handle to keep track of created task */
    0);                 /* pin task to core 0 */          
  delay(500); 

  Serial.print("Initialising TaskLoopCore1... ");
  xTaskCreatePinnedToCore(
    LoopCore1,            /* Task function. */
    "TaskLoopCore1",      /* name of task. */
    10000,                /* Stack size of task */
    NULL,                 /* parameter of the task */
    1,                    /* priority of the task */
    &TaskLoopCore1,       /* Task handle to keep track of created task */
    1);                   /* pin task to core 1 */
  delay(500); 
}

void loop() {
  
}

void LoopCore0( void * pvParameters ){
  Serial.print("TaskLoopCore0 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    digitalWrite(RED_LED, HIGH);
    delay(1000);
    digitalWrite(RED_LED, LOW);
    delay(1000);
  }
}

/* 
 * This task is pinned to core 1
 * It is used to monitor the state of the switches
*/

void LoopCore1( void * pvParameters ){
  Serial.print("TaskLoopCore1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    checkSwitch(MOTOR_SWITCH, motorSwitchState, []() {
    Serial.println("Motor Switch Pressed");
    // Additional actions to perform when the motor switch is pressed
  });

  checkSwitch(BRIGHTNESS_SWITCH, brightnessSwitchState, []() {
    Serial.println("Brightness Switch Pressed");
    // Additional actions to perform when the brightness switch is pressed
  });

  checkSwitch(COLOUR_SWITCH, colourSwitchState, []() {
    Serial.println("Colour Switch Pressed");
    // Additional actions to perform when the colour switch is pressed
  });

  checkSwitch(STATE_SWITCH, stateSwitchState, []() {
    Serial.println("State Switch Pressed");
    // Additional actions to perform when the state switch is pressed
  });
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
