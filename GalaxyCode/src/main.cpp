#include "main.h"

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
float rgbw_brightness = 0.0;
float moon_brightness = 255;

#define MAX_WIFI_ATTEMPTS 10 // Maximum number of WiFi connection attempts
#define WIFI_RETRY_DELAY 500 // Delay between WiFi connection attempts (in milliseconds)
bool wifiConnected = false; // Flag indicating WiFi connection status

#pragma region Function Declarations
void connectToWiFi(); // Function to connect to WiFi

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
#define POWER_SWITCH 26       // State switch
#pragma endregion

GenericFSM PowerFSM;
GenericFSM RGBWLedFSM;
GenericFSM MotorFSM;
GenericFSM BrightnessFSM;

//Switch objects
Switch stateSwitch(POWER_SWITCH, INPUT_PULLUP, PowerFSM);
Switch motorSwitch(MOTOR_SWITCH, INPUT_PULLUP, MotorFSM);
Switch brightnessSwitch(BRIGHTNESS_SWITCH, INPUT_PULLUP, BrightnessFSM);
Switch colourSwitch(COLOUR_SWITCH, INPUT_PULLUP, RGBWLedFSM);

RGBWLED rgbwLed;
PWM_Device moonProjectorLed;
PWM_Device motor;

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

  rgbwLed = RGBWLED(RED_LED, GREEN_LED, BLUE_LED, WHITE_LED);
  moonProjectorLed = PWM_Device(PROJECTOR_LED);
  motor = PWM_Device(MOTOR_BJT);

  // Initialise the state actions
  initPowerFSM();
  initRGBWLedFSM();
  initMotorFSM();
  initBrightnessFSM();

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
    OutputLoop,     /* Task function. */
    "TaskLoopCore1",      /* name of task. */
    10000,                /* Stack size of task */
    NULL,                 /* parameter of the task */
    1,                    /* priority of the task */
    &TaskLoopCore0,       /* Task handle to keep track of created task */
    1);                   /* pin task to core 1 */
  delay(500);

  Serial.print("Initialising TaskLoopCore0... ");
  xTaskCreatePinnedToCore(
    ButtonLoop,      /* Task function. */
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
void OutputLoop(void *pvParameters) {
  // Print the core ID for debugging purposes
  Serial.print("TaskLoopCore1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;) {
    // Handle power state regardless of other states
    PowerFSM.performStateAction();

    // Skip handling other states if the device is powered off
    if (strcmp(PowerFSM.getCurrentState().name.c_str(), "Power Off") == 0) {
      continue;
    }

    // Handle brightness state
    BrightnessFSM.performStateAction();

    // Handle RGBW state
    RGBWLedFSM.performStateAction();

    // Handle motor state
    MotorFSM.performStateAction();
  }
}

/*
  * Initialises the actions for the PowerFSM.
*/
void initPowerFSM() {
  State PowerOff = State("Power Off", []() {
    // If the power is off, turn everything off
    rgbwLed.set(0, 0, 0, 0);
    moonProjectorLed.set(0);
    motor.set(0);
  });
  PowerFSM.addState(PowerOff);

  State PowerOn = State("Power On", []() {/* Empty function since this is used as a flag */});
  PowerFSM.addState(PowerOn);

  State Project = State("Project", []() {
    moonProjectorLed.set(moon_brightness);
  });
  PowerFSM.addState(Project);
}

/*
  * Initialises the actions for the RGBWLedFSM.
*/
void initRGBWLedFSM() {
  State Blue = State("Blue", []() {
    rgbwLed.set(rgbw_brightness, 0, 0, 0);
  });
  RGBWLedFSM.addState(Blue);

  State Red = State("Red", []() {
    rgbwLed.set(0, rgbw_brightness, 0, 0);
  });
  RGBWLedFSM.addState(Red);

  State Green = State("Green", []() {
    rgbwLed.set(0, 0, rgbw_brightness, 0);
  });
  RGBWLedFSM.addState(Green);

  State White = State("White", []() {
    rgbwLed.set(0, 0, 0, rgbw_brightness);
  });
  RGBWLedFSM.addState(White);

  State BlueRed = State("Blue Red", []() {
    rgbwLed.set(rgbw_brightness, rgbw_brightness, 0, 0);
  });
  RGBWLedFSM.addState(BlueRed);

  State BlueGreen = State("Blue Green", []() {
    rgbwLed.set(rgbw_brightness, 0, rgbw_brightness, 0);
  });
  RGBWLedFSM.addState(BlueGreen);

  State RedGreen = State("Red Green", []() {
    rgbwLed.set(0, rgbw_brightness, rgbw_brightness, 0);
  });
  RGBWLedFSM.addState(RedGreen);

  State RedWhite = State("Red White", []() {
    rgbwLed.set(0, rgbw_brightness, 0, rgbw_brightness);
  });
  RGBWLedFSM.addState(RedWhite);

  State GreenWhite = State("Green White", []() {
    rgbwLed.set(0, 0, rgbw_brightness, rgbw_brightness);
  });
  RGBWLedFSM.addState(GreenWhite);

  State RedGreenBlue = State("Red Green Blue", []() {
    rgbwLed.set(0, rgbw_brightness, rgbw_brightness, rgbw_brightness);
  });
  RGBWLedFSM.addState(RedGreenBlue);

  State BlueGreenWhite = State("Blue Green White", []() {
    rgbwLed.set(rgbw_brightness, 0, rgbw_brightness, rgbw_brightness);
  });
  RGBWLedFSM.addState(BlueGreenWhite);

  State BlueRedGreenWhite = State("Blue Red Green White", []() {
    rgbwLed.set(rgbw_brightness, rgbw_brightness, rgbw_brightness, rgbw_brightness);
  });
  RGBWLedFSM.addState(BlueRedGreenWhite);

  State Cycle = State("Cycle", []() {
    rgbwLed.set(
      rgbw_brightness * (0.5 * (1 + sin(millis() / 1000.0))),               // Red
      rgbw_brightness * (0.5 * (1 + sin(millis() / 1000.0 + 2 * PI / 3))),  // Green
      rgbw_brightness * (0.5 * (1 + sin(millis() / 1000.0 + 4 * PI / 3))),  // Blue
      0); 
  });
  RGBWLedFSM.addState(Cycle);

  State Pulse = State("Pulse", []() {
    float brightness = rgbw_brightness * (0.5 * (1 + sin(millis() / 1000.0)));
    rgbwLed.set(brightness, brightness, brightness, brightness);      
  });
  RGBWLedFSM.addState(Pulse);
}

/*
  * Initialises the actions for the MotorFSM.
*/
void initMotorFSM() {
  State MotorOff = State("Motor Off", []() {
    motor.set(0);
  });
  MotorFSM.addState(MotorOff);

  State MotorFast = State("Motor Fast", []() {
    motor.set(255);
  });
  MotorFSM.addState(MotorFast);

  State MotorSlow = State("Motor Slow", []() {
    motor.set(100);
  });
  MotorFSM.addState(MotorSlow);
}

/* 
  * Initialises the actions for the BrightnessFSM.
*/
void initBrightnessFSM() {
  State BrightnessExtraLow = State("Extra Low", []() {
    rgbw_brightness = 20;
    moon_brightness = 20;
  });
  BrightnessFSM.addState(BrightnessExtraLow);

  State BrightnessLow = State("Low", []() {
    rgbw_brightness = 50;
    moon_brightness = 50;
  });
  BrightnessFSM.addState(BrightnessLow);

  State BrightnessMedium = State("Medium", []() {
    rgbw_brightness = 150;
    moon_brightness = 150;
  });
  BrightnessFSM.addState(BrightnessMedium);

  State BrightnessHigh = State("High", []() {
    rgbw_brightness = 255;
    moon_brightness = 255;
  });
  BrightnessFSM.addState(BrightnessHigh);
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
void ButtonLoop( void * pvParameters ){
  Serial.print("TaskLoopCore0 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;) {
    motorSwitch.update();
    brightnessSwitch.update();
    colourSwitch.update();
    stateSwitch.update();
    
    // Delay for 10ms to prevent the task from hogging the CPU
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void onStateChange() {
  // Guard against sending WebSocket messages before the connection is established
  if(!wifiConnected) return; // If WiFi is not connected, WebSocket is not initialised
  if(!ws.count()) return; // If there are no connected clients, update is not required
  updateClients(); // Update the connected clients
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
    PowerFSM.nextState();
  } else if (message == "Brightness") {
    BrightnessFSM.nextState();
  } else if (message == "Colour") {
    RGBWLedFSM.nextState();
  } else if (message == "Motor") {
    MotorFSM.nextState();
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
  states["Power"] = static_cast<int>(PowerFSM.getCurrentStateIndex());
  states["Brightness"] = static_cast<int>(BrightnessFSM.getCurrentStateIndex());
  states["Colour"] = static_cast<int>(RGBWLedFSM.getCurrentStateIndex());
  states["Motor"] = static_cast<int>(MotorFSM.getCurrentStateIndex());

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