#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

AsyncWebServer server(80);
TaskHandle_t Task1;
TaskHandle_t Task2;

#pragma region Function Declarations
void Task1code( void * pvParameters );
void Task2code( void * pvParameters );
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

int ledStates = 0b100000;

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

  Serial.println("Ready");

  Serial.print("Initialising Task1");
  xTaskCreatePinnedToCore(
             Task1code, /* Task function. */
             "Task1",   /* name of task. */
             10000,     /* Stack size of task */
             NULL,      /* parameter of the task */
             1,         /* priority of the task */
             &Task1,    /* Task handle to keep track of created task */
             0);        /* pin task to core 0 */          
  delay(500); 

  Serial.print("Initialising Task2");
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
    delay(500); 
}

void loop() {
  
}

void Task1code( void * pvParameters ){
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    digitalWrite(RED_LED, HIGH);
    delay(1000);
    digitalWrite(RED_LED, LOW);
    delay(1000);
  }
}

void Task2code( void * pvParameters ){
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  for(;;){
    digitalWrite(GREEN_LED, HIGH);
    delay(700);
    digitalWrite(GREEN_LED, LOW);
    delay(700);
  }
}