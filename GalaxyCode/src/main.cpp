#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Arduino_Json.h>
#include <iostream>
#include <map>
#include <functional>

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

// Maximum number of WiFi connection attempts
#define MAX_WIFI_ATTEMPTS 10
// Delay between WiFi connection attempts (in milliseconds)
#define WIFI_RETRY_DELAY 500
// Flag indicating WiFi connection status
bool wifiConnected = false;

#pragma region Function Declarations
// Core task function declarations
void OutputLoop(void *pvParameters);
void ButtonLoop(void *pvParameters);

// Function to connect to WiFi
void connectToWiFi();

// State handling function declarations
void initPowerFSM_Actions();
void initRGBWLedFSM_Actions();
void initMotorFSM_Actions();
void initBrightnessFSM_Actions();

void setRGBWLed(int red, int green, int blue, int white);
void handlePowerState();
void handleRGBWState();
void handleMotorState();
void handleBrightnessState();

// Switch handling function declarations
void onStateChange();


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
#define MOTOR_SWITCH 26       // Motor switch
#define BRIGHTNESS_SWITCH 25  // Brightness switch
#define COLOUR_SWITCH 33      // Colour switch
#define POWER_SWITCH 32       // State switch
#pragma endregion

#pragma region State Definitions
// Enumerations for various states
enum PowerStates {
  PowerOff,
  On,
  Project,
  Count,
  InitialState = PowerOff
};
GenericFSM<PowerStates> PowerFSM(PowerStates::InitialState, onStateChange);

struct RGBWState {
  int red;
  int green;
  int blue;
  int white;
};

enum RGBWLedStates {
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
  Count,
  InitialState = Blue
};
GenericFSM<RGBWLedStates> RGBWLedFSM(RGBWLedStates::InitialState, onStateChange);

enum MotorStates {
  MotorOff,
  Fast,
  Slow,
  Count,
  InitialState = MotorOff
};
GenericFSM<MotorStates> MotorFSM(MotorStates::InitialState, onStateChange);

enum BrightnessStates {
  ExtraLow,
  Low,
  Medium,
  High,
  Count,
  InitialState = ExtraLow
};
GenericFSM<BrightnessStates> BrightnessFSM(BrightnessStates::InitialState, onStateChange);

//Switch objects
Switch<PowerStates> stateSwitch(POWER_SWITCH, INPUT_PULLUP, PowerFSM);
Switch<MotorStates> motorSwitch(MOTOR_SWITCH, INPUT_PULLUP, MotorFSM);
Switch<BrightnessStates> brightnessSwitch(BRIGHTNESS_SWITCH, INPUT_PULLUP, BrightnessFSM);
Switch<RGBWLedStates> colourSwitch(COLOUR_SWITCH, INPUT_PULLUP, RGBWLedFSM);
#pragma endregion

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

  rgbwLed = new RGBWLED(RED_LED, GREEN_LED, BLUE_LED, WHITE_LED);
  moonProjectorLed = new PWM_Device(PROJECTOR_LED);
  motor = new PWM_Device(MOTOR_BJT);

  // Initialise the state actions
  initPowerFSM_Actions();
  initRGBWLedFSM_Actions();
  initMotorFSM_Actions();
  initBrightnessFSM_Actions();

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
bool initOutput = false;
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
  if (!initOutput) {
    // Print the core ID for debugging purposes
    Serial.print("TaskLoopCore1 running on core ");
    Serial.println(xPortGetCoreID());
    initOutput = true;
  }
  // Handle power state regardless of other states
  PowerFSM.performStateAction();

  // Skip handling other states if the device is powered off
  if (PowerFSM.getCurrentState() == PowerStates::PowerOff) {
    return;
  }

  // Handle brightness state
  BrightnessFSM.performStateAction();

  // Handle RGBW state
  RGBWLedFSM.performStateAction();

  // Handle motor state
  MotorFSM.performStateAction();
}

/*
  * Initialises the actions for the PowerFSM.
*/
void initPowerFSM_Actions() {
  // If the power is off, turn everything off
  PowerFSM.addStateAction(PowerStates::PowerOff, []() {
    rgbwLed.setRGBW(0, 0, 0, 0);
    moonProjectorLed.set(0);
    motor.set(0);
  });

  PowerFSM.addStateAction(PowerStates::Project, []() {
    moonProjectorLed.set(moon_brightness);
  });
}

/*
  * Initialises the actions for the RGBWLedFSM.
*/
void initRGBWLedFSM_Actions() {
  RGBWLedFSM.addStateAction(RGBWLedStates::Blue, []() {
    rgbwLed.setRGBW(0, 0, 255, 0);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::Red, []() {
    rgbwLed.setRGBW(255, 0, 0, 0);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::Green, []() {
    rgbwLed.setRGBW(0, 255, 0, 0);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::White, []() {
    rgbwLed.setRGBW(0, 0, 0, 255);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::BlueRed, []() {
    rgbwLed.setRGBW(255, 0, 255, 0);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::BlueGreen, []() {
    rgbwLed.setRGBW(0, 255, 255, 0);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::RedGreen, []() {
    rgbwLed.setRGBW(255, 255, 0, 0);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::RedWhite, []() {
    rgbwLed.setRGBW(255, 0, 0, 255);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::GreenWhite, []() {
    rgbwLed.setRGBW(0, 255, 0, 255);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::RedGreenBlue, []() {
    rgbwLed.setRGBW(255, 255, 255, 0);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::BlueGreenWhite, []() {
    rgbwLed.setRGBW(0, 255, 255, 255);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::BlueRedGreenWhite, []() {
    rgbwLed.setRGBW(255, 255, 255, 255);
  });

  RGBWLedFSM.addStateAction(RGBWLedStates::Cycle, []() {
    // Cycle through the colours using a sine wave on millis()
    rgbwLed.setRGBW(
      127.5 * (1 + sin(millis() / 1000.0)),               // Red
      127.5 * (1 + sin(millis() / 1000.0 + 2 * PI / 3)),  // Green
      127.5 * (1 + sin(millis() / 1000.0 + 4 * PI / 3)),  // Blue
      0);                                                 // White
  });
}

/*
  * Initialises the actions for the MotorFSM.
*/
void initMotorFSM_Actions() {
  MotorFSM.addStateAction(MotorStates::MotorOff, []() {
    motor.set(0);
  });

  MotorFSM.addStateAction(MotorStates::Fast, []() {
    motor.set(255);
  });

  MotorFSM.addStateAction(MotorStates::Slow, []() {
    motor.set(100);
  });
}

/* 
  * Initialises the actions for the BrightnessFSM.
*/
void initBrightnessFSM_Actions() {
  BrightnessFSM.addStateAction(BrightnessStates::ExtraLow, []() {
    rgbw_brightness = 20;
    moon_brightness = 20;
  });

  BrightnessFSM.addStateAction(BrightnessStates::Low, []() {
    rgbw_brightness = 100;
    moon_brightness = 100;
  });

  BrightnessFSM.addStateAction(BrightnessStates::Medium, []() {
    rgbw_brightness = 150;
    moon_brightness = 150;
  });

  BrightnessFSM.addStateAction(BrightnessStates::High, []() {
    rgbw_brightness = 255;
    moon_brightness = 255;
  });
}
#pragma endregion

#pragma region State Handlers
bool initButton = false;
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
  if (!initButton) {
    Serial.print("TaskLoopCore1 running on core ");
    Serial.println(xPortGetCoreID());
    initButton = true;
  }
  
  motorSwitch.update();
  brightnessSwitch.update();
  colourSwitch.update();
  stateSwitch.update();
  
  // Delay for 10ms to prevent the task from hogging the CPU
  vTaskDelay(10 / portTICK_PERIOD_MS);
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
  states["Power"] = static_cast<int>(PowerFSM.getCurrentState());
  states["Brightness"] = static_cast<int>(BrightnessFSM.getCurrentState());
  states["Colour"] = static_cast<int>(RGBWLedFSM.getCurrentState());
  states["Motor"] = static_cast<int>(MotorFSM.getCurrentState());

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

#pragma region Objects
/*
  * A class representing a switch and its functionality.
  * This class is used to handle switch debouncing and state changes.
*/
template <typename EnumType>
class Switch {
  public:
    Switch(uint8_t pin, uint8_t mode, GenericFSM<EnumType>& fsm) {
      this->pin = pin;
      this->fsm = fsm;
      pinMode(pin, mode);
    }

    /*
      * Checks the state of the switch and updates the switch state accordingly.
      * Must be called in the main loop.
    */
    void update() {
      unsigned long lastDebounceTime = 0;
      unsigned long timeReleased = 0;
      const unsigned long debounceDelay = 100;   // Debounce delay in milliseconds
      unsigned long currentMillis = millis();

      int switchReading = digitalRead(pin);

      if (switchReading == LOW) { // If the switch is pressed
        if (!switchState) {
          switchState = true; // Update the switch state
          fsm.nextState(); // Advance to the next state in the FSM

          // Print the current state to the serial monitor
          Serial.print("Switch Pressed - ");
          Serial.println(static_cast<int>(fsm.getCurrentState()));
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

  private:
    uint8_t pin;
    bool switchState = false;
    GenericFSM<EnumType>& fsm;
};

/**
 * @brief A generic Finite State Machine (FSM) template.
 *
 * @tparam StateType The type representing states (an enum).
 */
template<typename StateType>
class GenericFSM {
public:
    using TransitionFunction = std::function<void()>;
    using StateChangeCallback = std::function<void(StateType)>;

    /**
     * @brief Initializes the FSM with an enum of states.
     *
     * @param initialState The initial state of the FSM.
     * @param onStateChangeCallback A callback function to be invoked when the state changes.
     */
    explicit GenericFSM(StateType initialState, std::function<void()> onStateChangeCallback = nullptr) 
      : currentState(initialState), onStateChangeCallback(onStateChangeCallback) {} // Explicit means that the constructor cannot be used for implicit conversions and copy-initialization

    /**
     * @brief Advances to the next state in the enum.
     */
    void nextState() {
        // Calculate the next state based on the current state
        currentState = static_cast<StateType>((static_cast<int>(currentState) + 1) % static_cast<int>(StateType::Count));
        onStateChange(); // Invoke the callback function
    }

    /**
     * @brief Sets the state to a specified state.
     *
     * @param newState The state to set.
     */
    void setState(StateType newState) {
        currentState = newState;
        onStateChange(); // Invoke the callback function
    }

    /**
     * @brief Retrieves the current state of the FSM.
     *
     * @return The current state.
     */
    StateType getCurrentState() const {
        return currentState;
    }

    /**
     * @brief Adds an action to be performed when in a specified state.
     *
     * @param state The state to add the action to.
     * @param action The action to be performed.
     */
    void addStateAction(StateType state, TransitionFunction action) {
        stateActions[state] = action;
    }

    /**
     * @brief performs the action associated with the current state.
     */
    void performStateAction() {
        // If the current state does not have an associated action, return
        if (stateActions.find(currentState) == stateActions.end()) {
            return;
        }
        // Invoke the action associated with the current state
        stateActions[currentState]();
    }

private:
    StateType currentState;
    StateChangeCallback onStateChangeCallback; // A callback function to be invoked when the state changes
    std::map<StateType, TransitionFunction> stateActions; // A map of states and their corresponding actions

    void onStateChange() {
        // Invoke the callback function if it is not null
        if (onStateChangeCallback != nullptr) {
            onStateChangeCallback();
        }
    }
};

/*
  * A class representing an LED and its operations.
*/
class PWM_Device {
  public:
    PWM_Device(int pin) {
      this->pin = pin;
      pinMode(pin, OUTPUT);
    }

    void set(uint8_t value) {
      if (value == brightness) return; // If the brightness is unchanged, skip the operation
      if (value < 0 || value > 255) throw "Invalid PWM_Device value - must be between 0 and 255"; // If the brightness is out of range, throw an error
      
      brightness = value; // Update the brightness
      analogWrite(pin, brightness); // Set the brightness
    }

  private:
    int pin;
    int brightness = 0;
};

class RGBWLED {
  public:
    RGBWLED(int redPin, int greenPin, int bluePin, int whitePin) {
      redLED = new PWM_Device(redPin);
      greenLED = new PWM_Device(greenPin);
      blueLED = new PWM_Device(bluePin);
      whiteLED = new PWM_Device(whitePin);
    }
    
    ~RGBWLED() {
      delete redLED;
      delete greenLED;
      delete blueLED;
      delete whiteLED;
    }

    void set(int red, int green, int blue, int white) {
      redLED->set(red);
      greenLED->set(green);
      blueLED->set(blue);
      whiteLED->set(white);
    }

  private:
    PWM_Device* redLED;
    PWM_Device* greenLED;
    PWM_Device* blueLED;
    PWM_Device* whiteLED;
};
#pragma endregion