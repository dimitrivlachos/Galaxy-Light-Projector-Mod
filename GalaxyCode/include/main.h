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