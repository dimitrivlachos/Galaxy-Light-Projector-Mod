#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// Define the pin that the LED is connected to
#define LED_PIN D4

// Define the delay between blinks
#define BLINK_DELAY_MS 1000

// Setup function runs once when the board is powered on or reset
void setup() {
  #pragma region OTA Setup
  // OTA Initialization
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Once wifi connected, define
  ArduinoOTA.setHostname("Galaxy Night Light");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  #pragma endregion
  
  // Initialize the LED pin as an output
  pinMode(LED_PIN, OUTPUT);
}

// Loop function runs over and over again forever
void loop() {
  // Handle OTA functions
  ArduinoOTA.handle();

  // Turn the LED on
  digitalWrite(LED_PIN, HIGH);

  // Wait for the specified delay
  delay(BLINK_DELAY_MS);

  // Turn the LED off
  digitalWrite(LED_PIN, LOW);

  // Wait for the specified delay
  delay(BLINK_DELAY_MS);
}