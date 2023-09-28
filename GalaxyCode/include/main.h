#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Arduino_Json.h>
#include <iostream>
#include <map>
#include <functional>
#include <string>

#include "pwm_device.h"
#include "rgbw_led.h"
#include "generic_fsm.h"
#include "switch.h"
#include "state.h"


// Function declarations
void OutputLoop(void *pvParameters);
void ButtonLoop(void *pvParameters);

// State handling function declarations
void initPowerFSM();
void initRGBWLedFSM();
void initMotorFSM();
void initBrightnessFSM();
void onStateChange();

void connectToWiFi();

// WebSocket handling function declarations
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *payload, size_t length);
void initWebSocket();
String generateJsonForStates();
void updateClients();

// Pin definitions for various components
#define RED_LED 19            // Green wire
#define WHITE_LED 18          // Blue wire
#define GREEN_LED 17          // White wire
#define BLUE_LED 21           // Red wire
#define PROJECTOR_LED 27      // Moon projector
#define MOTOR_BJT 4           // Motor control
#define MOTOR_SWITCH 26       // Motor switch
#define BRIGHTNESS_SWITCH 25  // Brightness switch
#define COLOUR_SWITCH 33      // Colour switch
#define POWER_SWITCH 32       // State switch