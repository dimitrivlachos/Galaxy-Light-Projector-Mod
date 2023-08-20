#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Arduino_Json.h>

// Define and initialize the AsyncWebServer instance
AsyncWebServer server(80);
// Define and initialize the AsyncWebSocket instance
AsyncWebSocket ws("/ws");

// Task handles for the two core loops
TaskHandle_t TaskLoopCore0;
TaskHandle_t TaskLoopCore1;

// Switch state variables
bool motorSwitchState = false;
bool brightnessSwitchState = false;
bool colourSwitchState = false;
bool stateSwitchState = false;

// Brightness level variable
float brightness = 0.0;

// Maximum number of WiFi connection attempts
#define MAX_WIFI_ATTEMPTS 10
// Delay between WiFi connection attempts (in milliseconds)
#define WIFI_RETRY_DELAY 500
// Flag indicating WiFi connection status
bool wifiConnected = false;

#pragma region Function Declarations
// Core task function declarations
void LoopOutputHandle(void *pvParameters);
void LoopStateHandle(void *pvParameters);

// Function to connect to WiFi
void connectToWiFi();

// State handling function declarations
void setRGBWLed(int red, int green, int blue, int white);
void handlePowerState();
void handleRGBWState();
void handleMotorState();
void handleBrightnessState();

// Switch handling function declarations
void checkSwitch(int switchPin, bool &switchState, void (*callback)());
void handleStateSwitch();
void handleMotorSwitch();
void handleBrightnessSwitch();
void handleColourSwitch();

// Template function for incrementing enums
template <typename T>
T incrementEnum(T &enumValue, T lastEnumValue);

// Template function for handling switch state changes
template <typename EnumType>
void handleSwitch(EnumType &enumState, EnumType lastEnumValue, const char *switchName);

// WebSocket handling function declarations
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *payload, size_t length);
void initWebSocket();
String generateJsonForStates();
void updateClients();
#pragma endregion

#pragma region Wifi Settings
// WiFi configuration settings
const char *SSID = "ssid";
const char *PASSWORD = "pass";
const char *HOSTNAME = "GalaxyProjector-Dev";
const IPAddress STATIC_IP(192, 168, 0, 50);
const IPAddress GATEWAY(192, 168, 0, 1);
const IPAddress SUBNET(255, 255, 255, 0);
#pragma endregion

#pragma region Pin Definitions
// Pin definitions for various components
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
// Enumerations for various states
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

#pragma region HTML
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Galaxy Projector</title>
  <meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {
      font-family: 'Arial', sans-serif;
      text-align: center;
      background-color: #111111;
      color: #ffffff;
    }
    h1 {
      font-size: 2rem;
      margin: 1rem;
    }
    h2 {
      font-size: 1.8rem;
      font-weight: bold;
      color: #00bfff;
    }
    .topnav {
      overflow: hidden;
      background-color: #111111;
      padding: 1rem;
    }
    body {
      margin: 0;
    }
    .content {
      padding: 2rem;
      max-width: 800px;
      margin: 0 auto;
    }
    .card {
      background-color: #1a1a1a;
      box-shadow: 0 0 10px rgba(255, 255, 255, 0.2);
      padding: 1rem;
      border-radius: 10px;
      text-align: center;
    }
    .button {
      padding: 15px 50px;
      font-size: 24px;
      text-align: center;
      outline: none;
      color: #fff;
      background-color: #00bfff;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    .button:active {
      background-color: #0099cc;
      box-shadow: 2px 2px 5px rgba(0, 0, 0, 0.3);
      transform: translateY(2px);
    }
    .state {
      font-size: 1.2rem;
      color: #8c8c8c;
    }
    .grid-container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
      gap: 20px;
      margin-top: 1rem;
    }
  </style>
</head>
<body>
  <div class="topnav">
    <h1>🌠 Galaxy Projector 🌌</h1>
  </div>
  <div class="content">
    <div class="grid-container">
      <div class="card">
        <h2>Power</h2>
        <p><button id="Power" class="button">⚡️</button></p>
        <p class="state" id="PowerState">State: </p>
      </div>
      <div class="card">
        <h2>Brightness</h2>
        <p><button id="Brightness" class="button">☀️</button></p>
        <p class="state" id="BrightnessState">State: </p>
      </div>
      <div class="card">
        <h2>Colour</h2>
        <p><button id="Colour" class="button">🌈</button></p>
        <p class="state" id="ColourState">State: </p>
      </div>
      <div class="card">
        <h2>Spin</h2>
        <p><button id="Motor" class="button">💫</button></p>
        <p class="state" id="MotorState">State: </p>
      </div>
      <!-- Add more cards here -->
    </div>
  </div>
    <script>
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;
      var statesDict = {
        "Power": ['🌑', '🌓', '🌕'],
        "Brightness": ['🌒', '🌓', '🌔', '🌕'],
        "Colour": ['🔵', '🔴', '🟢', '⚪️', '🔵🔴', '🔵🟢', '🔴🟢', '🔴⚪️', '🟢⚪️', '🔴🟢🔵', '🔵🟢⚪️', '🔵🔴🟢⚪️', '🔄'],
        "Motor": ['🛑', '🐇', '🐢']
      };
      window.addEventListener('load', onLoad);
  
      function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen    = onOpen;
        websocket.onclose   = onClose;
        websocket.onmessage = onMessage;
      }
  
      function onOpen(event) {
        console.log('Connection opened');
        // Update the state of the buttons with the data received from the server
        sendMessage('getStates');
      }
  
      function onClose(event) {
        console.log('Connection closed');
        setTimeout(initWebSocket, 2000);
      }
  
      function onMessage(event) {
        console.log(`Received a message from server: ${event.data}`);
        try {
          var jsonData = JSON.parse(event.data);
          updateStates(jsonData);
        } catch (error) {
          console.error('Error parsing JSON:', error);
        }
      }
  
      function updateStates(data) {
        for (var key in data) {
          var stateElement = document.getElementById(key + 'State');
          if (stateElement) {
            stateElement.textContent = 'State: ' + statesDict[key][data[key]];
          }
        }
      }
  
      function onLoad(event) {
        initWebSocket();
        initButtons();
      }
  
      function initButtons() {
        // Initialize the buttons with their respective ids and listeners
        addButtonListener("Power");
        addButtonListener("Brightness");
        addButtonListener("Colour");
        addButtonListener("Motor");
      }
  
      function addButtonListener(buttonId) {
        var button = document.getElementById(buttonId);
        button.addEventListener('click', function() {
          sendMessage(buttonId);
        });
        button.buttonId = buttonId; // Store the button's id as a property for later use
      }
  
      function sendMessage(buttonId) {
        websocket.send(buttonId); // Send the button's id as a message to the ESP
      }
    </script>
</body>
</html>
)rawliteral";
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
    // Initialise WebSocket
    Serial.println("Initialising WebSocket");
    initWebSocket();
    Serial.println("WebSocket initialised");

    // Initialise OTA
    Serial.println("Initialising OTA");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", index_html);
    });
    AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
    server.begin();
    Serial.println("HTTP server started");
    Serial.println("OTA initialised");
  }
  else {
    Serial.println("WiFi not connected, OTA not initialised. WebSocket not initialised. Continuing offline...");
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
    LoopStateHandle,      /* Task function. */
    "TaskLoopCore0",      /* name of task. */
    10000,                /* Stack size of task */
    NULL,                 /* parameter of the task */
    1,                    /* priority of the task */
    &TaskLoopCore1,       /* Task handle to keep track of created task */
    0);                   /* pin task to core 0 */
  delay(500); 

  Serial.println("Tasks initialised");
  Serial.println("Setup complete!");
}

/**
 * Connects the device to a WiFi network using the provided SSID and password.
 * 
 * This function configures the WiFi mode, begins the connection process, sets the hostname,
 * and configures the IP address (if STATIC_IP is defined). It then attempts to connect
 * to the WiFi network, displaying connection progress. If the connection is successful,
 * the function updates the wifiConnected variable to true and prints the device's IP address.
 * If the connection fails, the function prints an error message, disconnects from WiFi,
 * and sets wifiConnected to false, allowing the device to continue offline.
 */
void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  
  // Set WiFi mode and begin connection
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
    wifiConnected = true; // Update wifiConnected status
  } else {
    Serial.println("\nFailed to connect to WiFi");
    WiFi.disconnect();    // Disconnect from WiFi
    wifiConnected = false; // Update wifiConnected status
    // Continue offline
    return;
  }
}

void loop() {
  ws.cleanupClients(); // Cleanup disconnected clients
}

#pragma region Output Handlers
/**
 * Task function to handle the output loop on a specific core.
 *
 * This task function is responsible for managing the output-related operations
 * of the device. It continuously handles the power state, brightness state,
 * RGBW state, and motor state according to their respective conditions.
 *
 * @param pvParameters A pointer to the parameters passed to the task (not used in this case).
 */
void LoopOutputHandle(void *pvParameters) {
  // Print the core ID for debugging purposes
  Serial.print("TaskLoopCore1 running on core ");
  Serial.println(xPortGetCoreID());

  // Enter the main loop
  for (;;) {
    // Handle power state regardless of other states
    handlePowerState();

    // Skip handling other states if the device is powered off
    if (pStates == PowerStateEnum::PowerOff) {
      continue;
    }

    // Handle brightness state
    handleBrightnessState();

    // Handle RGBW state
    handleRGBWState();

    // Handle motor state
    handleMotorState();
  }
}

/**
 * Handle power state and associated actions.
 *
 * This function is responsible for handling the power state of the device and
 * performing relevant actions based on the current power state. It controls the
 * LED colors, projector state, and motor state according to the power state.
 * If the power state is not recognized, an error message is printed to the serial monitor.
 *
 * @remarks The function behaviors in different power states:
 *   - PowerOff: Turns off all LEDs, deactivates the projector and motor.
 *   - On: Allows other state handlers to be executed, facilitating state transitions.
 *   - Project: Activates the projector and allows other state handlers to be executed.
 *     The projector LED will be on in this state.
 */
void handlePowerState() {
  switch (pStates) {
    case PowerStateEnum::PowerOff:
      // Turn off all LEDs and deactivate the projector and motor
      analogWrite(RED_LED, 0);
      analogWrite(GREEN_LED, 0);
      analogWrite(BLUE_LED, 0);
      analogWrite(WHITE_LED, 0);
      digitalWrite(PROJECTOR_LED, LOW);
      analogWrite(MOTOR_BJT, 0);
      break;
    case PowerStateEnum::On:
      // Being in this state allows the other states to be handled
      // Otherwise, the other state handlers will be skipped
      break;
    case PowerStateEnum::Project:
      // Activate the projector
      digitalWrite(PROJECTOR_LED, HIGH);
      // Being in this state allows the other states to be handled
      // Otherwise, the other state handlers will be skipped
      // In this case, the projector LED will be on as well.
      break;
    default:
      // Print an error message for unrecognized power state
      Serial.println("Invalid Power State");
      break;
  }
}

/**
 * Handle brightness state and set the global brightness level modifier.
 *
 * This function is responsible for adjusting the global brightness level modifier of the
 * device's LED colors based on the current brightness state. The brightness level
 * is controlled by modifying the 'brightness' variable. If the brightness state is
 * not recognized, an error message is printed to the serial monitor.
 *
 * @remarks The function behaviors in different brightness states:
 *   - ExtraLow: Sets the brightness to 25% of the maximum level.
 *   - Low: Sets the brightness to 50% of the maximum level.
 *   - Medium: Sets the brightness to 75% of the maximum level.
 *   - High: Sets the brightness to the maximum level (100%).
 */
void handleBrightnessState() {
  switch (bStates) {
    case BrightnessStateEnum::ExtraLow:
      // Set the brightness to 25% of the maximum level
      brightness = 0.25;
      break;
    case BrightnessStateEnum::Low:
      // Set the brightness to 50% of the maximum level
      brightness = 0.5;
      break;
    case BrightnessStateEnum::Medium:
      // Set the brightness to 75% of the maximum level
      brightness = 0.75;
      break;
    case BrightnessStateEnum::High:
      // Set the brightness to the maximum level (100%)
      brightness = 1;
      break;
    default:
      // Print an error message for unrecognized brightness state
      Serial.println("Invalid Brightness State");
      break;
  }
}

/**
 * Handle RGBW state and set the RGBW LED colors based on the current state.
 *
 * This function is responsible for setting the RGBW LED colors based on the current
 * RGBW state. The RGBW LED colors are determined by the 'rgbwStates' variable.
 * If the RGBW state is not recognized, an error message is printed to the serial monitor.
 */
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

/**
 * Handle motor state and control the motor speed based on the current state.
 *
 * This function is responsible for controlling the motor speed based on the current
 * motor state. The motor speed is controlled by adjusting the analog output value
 * to the MOTOR_BJT pin. If the motor state is not recognized, an error message is
 * printed to the serial monitor.
 *
 * @remarks The function behaviors in different motor states:
 *   - MotorOff: Turns off the motor by setting analog output to 0.
 *   - Fast: Sets the motor speed to maximum (255) using analog output.
 *   - Slow: Sets the motor speed to a moderate value (200) using analog output.
 */
void handleMotorState() {
  switch (mStates) {
    case MotorStateEnum::MotorOff:
      // Turn off the motor by setting analog output to 0
      analogWrite(MOTOR_BJT, 0);
      break;
    case MotorStateEnum::Fast:
      // Set the motor speed to maximum (255) using analog output
      analogWrite(MOTOR_BJT, 255);
      break;
    case MotorStateEnum::Slow:
      // Set the motor speed to a moderate value (200) using analog output
      analogWrite(MOTOR_BJT, 200);
      break;
    default:
      // Print an error message for unrecognized motor state
      Serial.println("Invalid Motor State");
      break;
  }
}

/**
 * Set the RGBW LED color and adjust brightness.
 *
 * This function sets the color of the RGBW LED by adjusting the analog output values
 * for each color channel (red, green, blue, white) based on the specified values
 * and the current brightness level. The brightness factor is multiplied with the
 * input color values to control the overall brightness of the LED.
 *
 * @param red The intensity of the red color channel (0-255).
 * @param green The intensity of the green color channel (0-255).
 * @param blue The intensity of the blue color channel (0-255).
 * @param white The intensity of the white color channel (0-255).
 */
void setRGBWLed(int red, int green, int blue, int white) {
  // Adjust the color intensity using the current brightness level
  analogWrite(RED_LED, red * brightness);
  analogWrite(GREEN_LED, green * brightness);
  analogWrite(BLUE_LED, blue * brightness);
  analogWrite(WHITE_LED, white * brightness);
}
#pragma endregion

#pragma region State Handlers
/**
 * Task function for monitoring and handling switch states.
 *
 * This task function is responsible for continuously monitoring the states of various switches
 * and invoking their corresponding handler functions when the switches are pressed or released.
 * The function includes a delay to prevent excessive CPU usage within the loop.
 *
 * @param pvParameters Pointer to task parameters (not used in this case).
 */
void LoopStateHandle( void * pvParameters ){
  Serial.print("TaskLoopCore1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    // Check the states of various switches and invoke their handlers
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
  // Skip handling the motor switch if the device is powered off
  if (pStates == PowerStateEnum::PowerOff) {
    return;
  }
  handleSwitch(mStates, MotorStateEnum::MotorLast, "Motor");
}

void handleBrightnessSwitch() {
  // Skip handling the motor switch if the device is powered off
  if (pStates == PowerStateEnum::PowerOff) {
    return;
  }
  handleSwitch(bStates, BrightnessStateEnum::BrightnessLast, "Brightness");
}

void handleColourSwitch() {
  // Skip handling the motor switch if the device is powered off
  if (pStates == PowerStateEnum::PowerOff) {
    return;
  }
  handleSwitch(rgbwStates, RGBWStateEnum::LedLast, "Colour");
}

/**
 * Handle switch state change for an enumerated state.
 *
 * This function is used to handle the change of an enumerated state associated with a switch press.
 * It increments the current state within the provided enumeration range and outputs information
 * about the switch press to the serial monitor.
 *
 * @param enumState A reference to the enumerated state variable to be modified.
 * @param lastEnumValue The last value in the enumeration range, used for wrapping.
 * @param switchName The name of the switch associated with this handler.
 */
template <typename EnumType>
void handleSwitch(EnumType &enumState, EnumType lastEnumValue, const char *switchName) {
  Serial.print(switchName);
  Serial.print(" Switch Pressed - ");
  incrementEnum(enumState, lastEnumValue);
  Serial.println(static_cast<int>(enumState));

  // Guard against sending WebSocket messages before the connection is established
  if(!wifiConnected) return; // If WiFi is not connected, WebSocket is not initialised
  if(!ws.count()) return; // If there are no connected clients, update is not required
    updateClients();
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

#pragma region Interface and WebSocket Handlers
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      Serial.printf("WebSocket client #%u data received\n", client->id());
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *payload, size_t length) {
  String message = "";
  for (size_t i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println(message);

  if (message == "Power") {
    handleStateSwitch();
  } else if (message == "Brightness") {
    handleBrightnessSwitch();
  } else if (message == "Colour") {
    handleColourSwitch();
  } else if (message == "Motor") {
    handleMotorSwitch();
  } else if (message == "getStates") {
    updateClients();
  } else {
    Serial.println("Invalid WebSocket message");
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String generateJsonForStates() {
  // Create a JSON object containing the current states
  JSONVar states;

  // Add the states to the JSON object
  states["Power"] = static_cast<int>(pStates);
  states["Brightness"] = static_cast<int>(bStates);
  states["Colour"] = static_cast<int>(rgbwStates);
  states["Motor"] = static_cast<int>(mStates);

  // Convert the JSON object to a string
  String json = JSON.stringify(states);

  // Return the JSON string
  return json;
}

void updateClients() {
  // Generate a JSON string containing the current states
  String json = generateJsonForStates();

  // Send the JSON string to all connected clients
  ws.textAll(json);
}
#pragma endregion