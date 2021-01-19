#ifndef _ESP32_TTGO_CO2_AMPEL_H_
#define _ESP32_TTGO_CO2_AMPEL_H_


#define DEBUG 1
#define DEBUG_MQTT 1
#include "debug_print.h"
#include "wifi_mqtt_creds.h"

// tft-display-control prototypes
void drawTrafficLight(int mode, bool clear=true);
void drawModeSelectMenu();
void drawConfigMenu(bool update=false);

int mqttConnect(bool updateDisplay=true);

#endif
