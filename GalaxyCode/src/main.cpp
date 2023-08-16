#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

AsyncWebServer server(80);

#pragma region Function Declarations
int myFunction(int, int);
void checkButtons();
#pragma endregion

#pragma region Wifi Settings
const char* SSID = "REPLACE_WITH_YOUR_SSID";
const char* PASSWORD = "REPLACE_WITH_YOUR_PASSWORD";
const char* HOSTNAME = "GalaxyProjector-Dev";
const IPAddress IP(192, 168, 0, 50);
const IPAddress GATEWAY(192, 168, 0, 1);
const IPAddress SUBNET(255, 255, 255, 0);
#pragma endregion

#pragma region Pin Definitions
#define GREEN_LED 17          // Green wire
#define BLUE_LED 18           // Blue wire
#define WHITE_LED 19          // White wire
#define BROWN_LED 21          // Brown wire
#define PROJECTOR_LED 27      // Moon projector
#define MOTOR_BJT 4           // Motor control
#define MOTOR_SWITCH 32       // Motor switch
#define BRIGHTNESS_SWITCH 33  // Brightness switch
#define COLOUR_SWITCH 25      // Colour switch
#define STATE_SWITCH 26       // State switch
#pragma endregion

#pragma region State Definitions
// State Definitions

#pragma endregion

int ledStates = 0b100000;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World!");

  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  pinMode(BROWN_LED, OUTPUT);
  pinMode(PROJECTOR_LED, OUTPUT);
  pinMode(MOTOR_BJT, OUTPUT);
  pinMode(MOTOR_SWITCH, INPUT_PULLUP);
  pinMode(BRIGHTNESS_SWITCH, INPUT_PULLUP);
  pinMode(COLOUR_SWITCH, INPUT_PULLUP);
  pinMode(STATE_SWITCH, INPUT_PULLUP);

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
}

float lastChangeTime = 0;

void loop() {
  if (millis() - lastChangeTime > 1000) {
    // Cycle through each LED, turning one on at a time
    digitalWrite(GREEN_LED, ledStates & 0b100000);
    digitalWrite(BLUE_LED, ledStates & 0b010000);
    digitalWrite(WHITE_LED, ledStates & 0b001000);
    digitalWrite(BROWN_LED, ledStates & 0b000100);
    digitalWrite(MOTOR_BJT, ledStates & 0b000010);
    digitalWrite(PROJECTOR_LED, ledStates & 0b000001);

    ledStates = ledStates >> 1;
    //Reset the LED states if all LEDs have been turned off
    if(ledStates == 0) {
      ledStates = 0b100000;
    }
    lastChangeTime = millis();
  }
  checkButtons();
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

void checkButtons() {
  if(digitalRead(MOTOR_SWITCH) == LOW) {
    Serial.println("Motor Switch Pressed");
  }
  if(digitalRead(BRIGHTNESS_SWITCH) == LOW) {
    Serial.println("Brightness Switch Pressed");
  }
  if(digitalRead(COLOUR_SWITCH) == LOW) {
    Serial.println("Colour Switch Pressed");
  }
  if(digitalRead(STATE_SWITCH) == LOW) {
    Serial.println("State Switch Pressed");
  }
}