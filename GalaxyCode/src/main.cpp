#include "main.h"

AsyncWebServer server(80); // Define and initialize the AsyncWebServer instance
AsyncWebSocket ws("/ws"); // Define and initialize the AsyncWebSocket instance

// Task handles for the two core loops
TaskHandle_t TaskLoopCore0;
TaskHandle_t TaskLoopCore1;

#define MAX_WIFI_ATTEMPTS 10 // Maximum number of WiFi connection attempts
#define WIFI_RETRY_DELAY 500 // Delay between WiFi connection attempts (in milliseconds)
bool wifiConnected = false; // Flag indicating WiFi connection status

// WiFi configuration settings
const char *SSID = "VM1732529";
const char *PASSWORD = "dqg9bbhkRbCq";
const char *HOSTNAME = "GalaxyProjector-Dev";
const IPAddress STATIC_IP(192, 168, 0, 49);
const IPAddress GATEWAY(192, 168, 0, 1);
const IPAddress SUBNET(255, 255, 255, 0);

GenericFSM PowerFSM("Power", onStateChange);
GenericFSM BrightnessFSM("Brightness", onStateChange);
GenericFSM RGBWLedFSM("Colour", onStateChange);
GenericFSM MotorFSM("Motor", onStateChange);

//Switch objects
Switch stateSwitch(POWER_SWITCH, INPUT_PULLUP, PowerFSM);
Switch motorSwitch(MOTOR_SWITCH, INPUT_PULLUP, MotorFSM);
Switch brightnessSwitch(BRIGHTNESS_SWITCH, INPUT_PULLUP, BrightnessFSM);
Switch colourSwitch(COLOUR_SWITCH, INPUT_PULLUP, RGBWLedFSM);

RGBWLED rgbwLed = RGBWLED(RED_LED, GREEN_LED, BLUE_LED, WHITE_LED);
PWM_Device moonProjectorLed = PWM_Device(PROJECTOR_LED);
PWM_Device motor = PWM_Device(MOTOR_BJT);

// State variables
float rgbw_brightness = 0.0;
float moon_brightness = 255;

struct RGBW_Values {
  int red = 0;
  int green = 0;
  int blue = 0;
  int white = 0;

  String toJson() {
    JSONVar json;
    json["red"] = red;
    json["green"] = green;
    json["blue"] = blue;
    json["white"] = white;
    return json;
    }
};
struct RGBW_Values rgbw_values;

struct Brightness_Values {
  private:
    float rgbw_brightness = 0.0;
    float moon_brightness = 1.0;

  public:
    void setRgbwBrightness(int value) {
      rgbw_brightness = map(value, 0, 255, 0.0, 1.0);
    }

    void setMoonBrightness(int value) {
      moon_brightness = map(value, 0, 255, 0.0, 1.0);
    }

    float getRgbwBrightness() {
      return rgbw_brightness;
    }

    float getMoonBrightness() {
      return moon_brightness;
    }
  String toJson() {
    JSONVar json;
    json["rgbw_brightness"] = rgbw_brightness;
    json["moon_brightness"] = moon_brightness;
    return json;
  }
};
struct Brightness_Values brightness_values;

struct Motor_Values {
  int speed = 0;

  String toJson() {
    return "{\"speed\": " + String(speed) + "}";
  }
};
struct Motor_Values motor_values;

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

    /* Toggle Switch */
    .switch {
      display: inline-block;
      position: relative;
      width: 60px;
      height: 34px;
    }

    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }

    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      -webkit-transition: .4s;
      transition: .4s;
      border-radius: 34px;
    }

    .slider:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      -webkit-transition: .4s;
      transition: .4s;
      border-radius: 50%;
    }

    input:checked+.slider {
      background-color: #2196F3;
    }

    input:focus+.slider {
      box-shadow: 0 0 1px #2196F3;
    }

    input:checked+.slider:before {
      -webkit-transform: translateX(26px);
      -ms-transform: translateX(26px);
      transform: translateX(26px);
    }

    /* Style for sliders when active */
    input[type="range"]:hover {
      opacity: 1;
    }

    /* Style for sliders when dragged */
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      background: #00bfff;
      cursor: pointer;
      border-radius: 50%;
    }

    input[type="range"]::-moz-range-thumb {
      width: 20px;
      height: 20px;
      background: #00bfff;
      cursor: pointer;
      border-radius: 50%;
    }

    /* Add margin between color sliders */
    .color-slider {
      margin-bottom: 15px;
    }

    .label {
      font-size: 1.2rem;
      font-weight: bold;
      display: inline-block;
      /* Make labels inline with the sliders */
      width: 30px;
      /* Adjust the width to your preference */
      text-align: center;
      /* Center-align the label text */
      margin-right: 10px;
      /* Add some spacing between labels and sliders */
    }
  </style>
</head>

<body>
  <div class="topnav">
    <h1>🌠 Galaxy Projector 🌌</h1>
  </div>
  <div class="content">
    <div class="grid-container">

      <!-- Cards -->
      <!-- Power -->
      <div class="card">
        <h2>Power</h2>
        <p><button id="PowerButton" class="button">⚡️</button></p>
        <p class="state" id="PowerState">State: </p>
      </div>

      <!-- Brightness -->
      <div class="card" id="BrightnessCard">
        <h2>Brightness</h2>
        <div id="BrightnessButtonsContainer" style="display: block;">
          <p><button id="BrightnessButton" class="button">☀️</button></p>
        </div>
        <div id="BrightnessSlidersContainer" style="display: none;">
          <h2>☀️</h2>
          <input type="range" id="BrightnessRange" min="0" max="100" value="50">
        </div>
        <!-- Brightness Mode Selector -->
        <div class="mode-selector">
          <label class="switch">
            <input type="checkbox" id="brightnessModeToggle">
            <span class="slider"></span>
          </label>
        </div>
        <p class="state" id="BrightnessState">State: </p>
      </div>

      <!-- Colour -->
      <div class="card" id="ColourCard">
        <h2>Colour</h2>
        <div id="ColourButtonsContainer" style="display: block;">
          <p><button id="ColourButton" class="button">🌈</button></p>
        </div>
        <div id="ColourSlidersContainer" style="display: none;">
          <h2>🌈</h2>
          <label class="label" for="RedSlider" style="color: #ff0000;">🔴</label>
          <input type="range" id="RedSlider" min="0" max="255" value="128" style="background-color: red;">
          <label class="label" for="GreenSlider" style="color: #00ff00;">🟢</label>
          <input type="range" id="GreenSlider" min="0" max="255" value="128">
          <label class="label" for="BlueSlider" style="color: #0000ff;">🔵</label>
          <input type="range" id="BlueSlider" min="0" max="255" value="128">
          <label class="label" for="WhiteSlider" style="color: #ffffff;">⚪</label>
          <input type="range" id="WhiteSlider" min="0" max="255" value="128">
        </div>
        <!-- Colour Mode Selector -->
        <div class="mode-selector">
          <label class="switch">
            <input type="checkbox" id="colourModeToggle">
            <span class="slider"></span>
          </label>
        </div>
        <p class="state" id="ColourState">State: </p>
      </div>

      <!-- Motor -->
      <div class="card" id="MotorCard">
        <h2>Spin</h2>
        <div id="MotorButtonsContainer" style="display: block;">
          <p><button id="MotorButton" class="button">💫</button></p>
        </div>
        <div id="MotorSlidersContainer" style="display: none;">
          <h2>💫</h2>
          <input type="range" id="MotorSlider" min="0" max="100" value="50">
        </div>
        <!-- Spin Mode Selector -->
        <div class="mode-selector">
          <label class="switch">
            <input type="checkbox" id="spinModeToggle">
            <span class="slider"></span>
          </label>
        </div>
        <p class="state" id="MotorState">State: </p>
      </div>

      <!-- Add more cards here -->
    </div>
  </div>
  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;
    var maxConnectionAttempts = 10;
    var connectionAttempts = 0;
    var statesDict = {
      "Power": ['🌑', '🌓', '🌕'],
      "Brightness": ['🌒', '🌓', '🌔', '🌕'],
      "Colour": ['🔵', '🔴', '🟢', '⚪️', '🔵🔴', '🔵🟢', '🔴🟢', '🔴⚪️', '🟢⚪️', '🔴🟢🔵', '🔵🟢⚪️', '🔵🔴🟢⚪️', '🔄', '💗'],
      "Motor": ['🛑', '🐇', '🐢']
    };
    window.addEventListener('load', onLoad);

    function initWebSocket() {
      console.log('Trying to open a WebSocket connection...');
      connectionAttempts++;
      websocket = new WebSocket(gateway);
      websocket.onopen = onOpen;
      websocket.onclose = onClose;
      websocket.onmessage = onMessage;
    }

    function onOpen(event) {
      console.log('Connection opened');
      connectionAttempts = 0;
      // Update the state of the buttons with the data received from the server
      websocket.send('getStates');
    }

    function onClose(event) {
      console.log('Connection closed');
      if (connectionAttempts < maxConnectionAttempts) {
        setTimeout(initWebSocket, 2000);
      } else {
        console.log('Maximum connection attempts reached');
      }
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
      // Data example:
      /* 
      {
        "Power":{"mode":"0","value":"2"},
        "Brightness":{"mode":"0","value":"2"},
        "Colour":{"mode":"0","value":"0"},
        "Motor":{"mode":"0","value":"0"}
      }
      */
      var Power = data['Power'];
      var Pmode = parseInt(Power['mode']);
      var Pvalue = parseInt(Power['value']);
      setModeState('Power', Pmode);
      var PowerState = document.getElementById('PowerState');
      if (Pmode == 0) {
        PowerState.textContent = 'State: ' + statesDict['Power'][Pvalue];
      } else {
        PowerState.textContent = 'State: ' + Pvalue;
      }

      var Brightness = data['Brightness'];
      var Bmode = parseInt(Brightness['mode']);
      var Bvalue = parseInt(Brightness['value']);
      setModeState('Brightness', Bmode);
      var BrightnessState = document.getElementById('BrightnessState');
      if (Bmode == 0) {
        BrightnessState.textContent = 'State: ' + statesDict['Brightness'][Bvalue];
      } else {
        BrightnessState.textContent = 'State: ' + Bvalue;
        // Set the brightness slider to the value received from the server
        var brightnessSlider = document.getElementById('BrightnessRange');
        brightnessSlider.value = Bvalue;
      }

      var Colour = data['Colour'];
      var Cmode = parseInt(Colour['mode']);
      var Cvalue = Colour['value'];
      setModeState('Colour', Cmode);
      var ColourState = document.getElementById('ColourState');
      if (Cmode == 0) {
        ColourState.textContent = 'State: ' + statesDict['Colour'][Cvalue];
      } else {
        ColourState.textContent = 'State: ' + Cvalue;
        // Set the colour sliders to the values received from the server
        var redSlider = document.getElementById('RedSlider');
        var greenSlider = document.getElementById('GreenSlider');
        var blueSlider = document.getElementById('BlueSlider');
        var whiteSlider = document.getElementById('WhiteSlider');
        redSlider.value = Cvalue[0];
        greenSlider.value = Cvalue[1];
        blueSlider.value = Cvalue[2];
        whiteSlider.value = Cvalue[3];
      }

      var Motor = data['Motor'];
      var Mmode = parseInt(Motor['mode']);
      var Mvalue = parseInt(Motor['value']);
      setModeState('Motor', Mmode);
      var MotorState = document.getElementById('MotorState');
      if (Mmode == 0) {
        MotorState.textContent = 'State: ' + statesDict['Motor'][Mvalue];
      } else {
        MotorState.textContent = 'State: ' + Mvalue;
        // Set the motor slider to the value received from the server
        var motorSlider = document.getElementById('MotorSlider');
        motorSlider.value = Mvalue;
      }
    }

    function setBrightnessSliderValue(value) {
      var brightnessSlider = document.getElementById('BrightnessRange');
      brightnessSlider.value = value;
    }

    function setColourSliderValues(values) {
      var redSlider = document.getElementById('RedSlider');
      var greenSlider = document.getElementById('GreenSlider');
      var blueSlider = document.getElementById('BlueSlider');
      var whiteSlider = document.getElementById('WhiteSlider');

      redSlider.value = values[0];
      greenSlider.value = values[1];
      blueSlider.value = values[2];
      whiteSlider.value = values[3];
    }

    function setMotorSliderValue(value) {
      var motorSlider = document.getElementById('MotorSlider');
      motorSlider.value = value;
    }

    function onLoad(event) {
      initWebSocket();
      initButtons();
      initModeSelectors();
      initSliderListeners();
    }

    function initButtons() {
      // Initialize the buttons with their respective ids and listeners
      addButtonListener("PowerButton", "Power");
      addButtonListener("BrightnessButton", "Brightness");
      addButtonListener("ColourButton", "Colour");
      addButtonListener("MotorButton", "Motor");
    }

    function addButtonListener(buttonId, messageId) {
      var button = document.getElementById(buttonId);
      button.addEventListener('click', function () {
        sendMessage(messageId, modes.set, 1);
      });
    }

    function initModeSelectors() {
      var brightnessModeToggle = document.getElementById('brightnessModeToggle');

      brightnessModeToggle.addEventListener('change', function () {
        var isBrightnessChecked = brightnessModeToggle.checked;
        setModeState('Brightness', isBrightnessChecked);
      });

      var colourModeToggle = document.getElementById('colourModeToggle');

      colourModeToggle.addEventListener('change', function () {
        var isColourChecked = colourModeToggle.checked;
        setModeState('Colour', isColourChecked);
      });

      var spinModeToggle = document.getElementById('spinModeToggle');

      spinModeToggle.addEventListener('change', function () {
        var isSpinChecked = spinModeToggle.checked;
        setModeState('Motor', isSpinChecked);
      });
    }

    function setModeState(cardId, mode) {
      if (cardId == 'Power') return; // Power card has no mode selector

      var ButtonsContainer = document.getElementById(cardId + 'ButtonsContainer');
      var SlidersContainer = document.getElementById(cardId + 'SlidersContainer');

      if (mode) {
        // Switch to range sliders
        ButtonsContainer.style.display = 'none';
        SlidersContainer.style.display = 'block';
      } else {
        // Switch back to buttons
        ButtonsContainer.style.display = 'block';
        SlidersContainer.style.display = 'none';
      }
    }

    // Function to initialize slider listeners
    function initSliderListeners() {
      // Get the Brightness slider element
      var brightnessSlider = document.getElementById('BrightnessRange');

      // Add an 'input' event listener to detect value changes
      brightnessSlider.addEventListener('input', function () {
        // Get the current value of the Brightness slider
        var brightnessValue = brightnessSlider.value;

        sendMessage("Brightness", modes.custom, brightnessValue);
      });

      // Get the colour sliders
      var redSlider = document.getElementById('RedSlider');
      var greenSlider = document.getElementById('GreenSlider');
      var blueSlider = document.getElementById('BlueSlider');
      var whiteSlider = document.getElementById('WhiteSlider');

      // Add an 'input' event listener to detect value changes
      redSlider.addEventListener('input', function () {
        sendColourValues(redSlider.value, greenSlider.value, blueSlider.value, whiteSlider.value);
      });

      greenSlider.addEventListener('input', function () {
        sendColourValues(redSlider.value, greenSlider.value, blueSlider.value, whiteSlider.value);
      });

      blueSlider.addEventListener('input', function () {
        sendColourValues(redSlider.value, greenSlider.value, blueSlider.value, whiteSlider.value);
      });

      whiteSlider.addEventListener('input', function () {
        sendColourValues(redSlider.value, greenSlider.value, blueSlider.value, whiteSlider.value);
      });

      // Get the Motor slider element
      var motorSlider = document.getElementById('MotorSlider');

      // Add an 'input' event listener to detect value changes
      motorSlider.addEventListener('input', function () {
        // Get the current value of the Motor slider
        var motorValue = motorSlider.value;

        sendMessage("Motor", modes.custom, motorValue);
      });
    }

    function sendColourValues(red, green, blue, white) {
      // String formatted array of colour values
      var colourValues = [red, green, blue, white];

      // Send the updated colour values to the WebSocket
      sendMessage("Colour", modes.custom, colourValues.join(''))
    }

    const RateLimit = 500; // ms
    var lastMessageTime = 0;

    function sendMessage(id, mode, value) {
      if (lastMessageTime && (Date.now() - lastMessageTime < RateLimit)) {
        return;
      }

      // If there is no connection with the WebSocket, return
      if (!websocket || websocket.readyState === 3) {
        console.log('WebSocket connection not established, message ' + id + ' not sent');
        return;
      }

      // Build the json as a string instead
      var message = "{\"id\":\"" + id + "\",\"mode\":\"" + mode + "\",\"value\":\"" + value + "\"}";

      console.log('Sending: ' + message);
      websocket.send(message); // Send the button's id as a message to the ESP
      lastMessageTime = Date.now();
    }

    const id = {
      'Power': 0,
      'Brightness': 1,
      'Colour': 2,
      'Motor': 3
    };

    // Enum for modes
    const modes = {
      'set': 0,
      'custom': 1
    };
  </script>
</body>

</html>
)rawliteral";
#pragma endregion

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

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

    rgbwLed.set(
      rgbw_values.red * brightness_values.rgbw_brightness,
      
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

  State customColours = State("Custom Colours", []() {
    // Set the RGBW values to the values stored in the custom state
    rgbwLed.set(rgbw_values.red, rgbw_values.green, rgbw_values.blue, rgbw_values.white);
  });
  RGBWLedFSM.setCustomState(customColours);
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

  State customSpeed = State("Custom Speed", []() {
    motor.set(customMotor_s.speed);
  });
  MotorFSM.setCustomState(customSpeed);
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

  State customBrightness = State("Custom Brightness", []() {
    // Set the brightness values to the values stored in the custom state
    rgbw_brightness = customBrightness_s.rgbw_brightness;
    moon_brightness = customBrightness_s.moon_brightness;
  });
  BrightnessFSM.setCustomState(customBrightness);
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

  Serial.println("Parsing: " + message);

  try {
    if (message == "getStates") {
      updateClients();
      return;
    }

    // Parse the JSON string
    JSONVar json = JSON.parse(message);

    // Get the id, mode, and value from the JSON object
    String id = json["id"];
    String mode = json["mode"];
    String value = json["value"];
  

    Serial.println("id: " + id);
    Serial.println("mode: " + mode);
    Serial.println("value: " + value);

    if (mode == "0") { // If the mode is set
      if (id == "Power") { // Power
        PowerFSM.nextState();
      } else if (id == "Brightness") { // Brightness
        BrightnessFSM.nextState();
      } else if (id == "Colour") { // Colour
        RGBWLedFSM.nextState();
      } else if (id == "Motor") { // Motor
        MotorFSM.nextState();
      }
    } 
    else if (mode == "1") { // If the mode is custom
      if (id == "Brightness") { // Brightness
        Serial.println("In brightness");
        int brightness = std::stoi(value.c_str());
        Serial.println("Brightness: " + brightness);
        customBrightness_s.rgbw_brightness = brightness;
        customBrightness_s.moon_brightness = brightness;
        
        BrightnessFSM.useCustomState();
      } else if (id == "Colour") { // Colour
        // Parse the JSON string
        // Example: {"id":"Colour","mode":1,"value":["128","170","128","128"]}
        //JSONVar json = JSON.parse(value);

        // Get the RGBW values from the JSON object

        rgbw_values.red = (int)value.substring(0, 3).c_str();
        rgbw_values.green = (int)value.substring(4, 7).c_str();
        rgbw_values.blue = (int)value.substring(8, 11).c_str();
        rgbw_values.white = (int)value.substring(12, 15).c_str();

        RGBWLedFSM.useCustomState();
      } else if (id == "Motor") { // Motor
        customMotor_s.speed = (int)value.c_str();
        MotorFSM.useCustomState();
      }
    }
  }
  catch (const std::exception& e) {
    Serial.println("Invalid WebSocket message");
    Serial.println(e.what());
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String generateJsonForStates() {
  // Create a JSON object containing the current states
  JSONVar fsmStates;

  fsmStates["Power"]["mode"] = PowerFSM.custom ? "1" : "0";
  fsmStates["Power"]["value"] = PowerFSM.custom ? "You should not be able to get this" : std::to_string(PowerFSM.getCurrentStateIndex()).c_str();

  fsmStates["Brightness"]["mode"] = BrightnessFSM.custom ? "1" : "0";
  fsmStates["Brightness"]["value"] = BrightnessFSM.custom ? customBrightness_s.toJson().c_str() : std::to_string(BrightnessFSM.getCurrentStateIndex()).c_str();

  fsmStates["Colour"]["mode"] = RGBWLedFSM.custom ? "1" : "0";
  fsmStates["Colour"]["value"] = RGBWLedFSM.custom ? rgbw_values.toJson().c_str() : std::to_string(RGBWLedFSM.getCurrentStateIndex()).c_str();

  JSONVar motorState;
  fsmStates["Motor"]["mode"] = MotorFSM.custom ? "1" : "0";
  fsmStates["Motor"]["value"] = MotorFSM.custom ? customMotor_s.toJson().c_str() : std::to_string(MotorFSM.getCurrentStateIndex()).c_str();

  // Convert the JSON object to a string
  String json = JSON.stringify(fsmStates);
  delete fsmStates;

  // Return the JSON string
  // Example:
  // {"0":{"mode":"0","value":"0"},"1":{"mode":"0","value":"0"},"2":{"mode":"0","value":"0"},"3":{"mode":"0","value":"0"}}
  return json;
}

void updateClients() {
  // Generate a JSON string containing the current states
  String json = generateJsonForStates();

  // Send the JSON string to all connected clients
  Serial.println(json);
  ws.textAll(json);
}
#pragma endregion