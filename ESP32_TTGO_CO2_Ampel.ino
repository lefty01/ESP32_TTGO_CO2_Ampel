/*--------------------------------------------------

  The MIT License (MIT)
  (c) 2019,2020,2022 andreas loeffler <al@exitzero.de>

  --------------------------------------------------*/



// CO2 Thresholds (ppm).
//
// Recommendation from REHVA (Federation of European Heating, Ventilation and
// Air Conditioning associations, rehva.eu)
// for preventing COVID-19 aerosol spread especially in schools:
// - warn: 800, critical: 1000
// (https://www.rehva.eu/fileadmin/user_upload/REHVA_COVID-19_guidance_document_V3_03082020.pdf)
//
// General air quality recommendation by the German Federal Environmental Agency (2008):
// - warn: 1000, critical: 2000
// (https://www.umweltbundesamt.de/sites/default/files/medien/pdfs/kohlendioxid_2008.pdf)

// firewall-cmd --zone=trusted --add-source=192.168.1.75


// TODO save mqttdeviceId in flash/eeprom
// calibrate feature via ttgo board button

#define VERSION "0.2.1"
//#define MQTTDEVICEID "co2_ampel"
#define OTA_HOSTNAME "co2_ampel"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <MHZ.h>
#include <EEPROM.h>
#include <esp_now.h>


//#include "NotoSansBold15.h"
// The font names are arrays references, thus must NOT be in quotes ""
//#define AA_FONT_SMALL NotoSansBold15

#include "ESP32_TTGO_CO2_Ampel.h"

#define SERVER_PORT 4088

String  MQTTDEVICEID = "co2_ampel_";
byte mac_addr[6];
uint8_t broadcastAddress[] = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc};

typedef struct message {
    int temp;
    int co2;
} message_t;
message_t txReadings;

bool lastEspNowSentStatus = false;

// todo: do we get this from tft.width() and tft.height()?
#define TFT_WIDTH  240
#define TFT_HEIGHT 128

#define ADC_EN          14  //ADC_EN is the ADC detection enable port
//  -> does this interfere with input/pullup???
#define ADC_PIN         34


// pin for pwm reading
//#define CO2_IN 10
// pin for uart reading
#define MH_Z19_RX 15
#define MH_Z19_TX 17
const unsigned long MHZ19B_PREHEATING_TIME = 3 * 60 * 1000;
const unsigned MHZ19B_PREHEATING_TIMEs = 3 * 60; // 3 minutes

#define DATA_PIN       2 // to rgb-strip Din
#define NUM_LEDS       8
#define CHIPSET        WS2812B
#define COLOR_ORDER    GRB
#define BRIGHTNESS      128
#define MAX_BRIGHTNESS  200 // for now
#define MIN_BRIGHTNESS    2 // for now

#define OTA_BUTTON       35

#define EVERY_SECOND 1000
#define EVERY_10_SECONDS 10 * EVERY_SECOND
#define EVERY_MINUTE EVERY_SECOND * 60
unsigned long sw_timer_10s;
unsigned long sw_timer_2s;
unsigned long sw_timer_4s;
unsigned long sw_timer_10ms;
unsigned long sw_timer_clock;

bool OTA_ENABLED = false;
const char* ssid = "CO2_AMPEL1";
const char* password = "m1u2r3c4s5";

// This is an array of leds.  One item for each led in your strip.
CRGB leds[NUM_LEDS];
// fastled palette/blend
CRGBPalette16 currentPalette;
TBlendType    currentBlending;

WiFiClientSecure net;
PubSubClient mqttClient(net);
TFT_eSPI tft = TFT_eSPI();

SoftwareSerial co2Serial(MH_Z19_RX, MH_Z19_TX); // define MH-Z19B

static volatile int alarmThreshold = 800; // keep in eeprom, make adjustable
static volatile int cleanThreshold = 700;

static volatile int warnThreshold = 800; // keep in eeprom, make adjustable
static volatile int criticalThreshold = 1000;

static volatile bool warnReached = false;
static volatile bool criticalReached = false;

static volatile int ppm_uart;
static volatile int temperature;
int data[2];

bool isWifiAvailable = false;
bool isMqttAvailable = false;
bool isTcpClient     = false;

String MQTT_TOPIC_STATE      = "%CHIP_ID%/state";
String MQTT_TOPIC_SETCONFIG  = "%CHIP_ID%/setconfig";
String MQTT_TOPIC_CO2PPM     = "%CHIP_ID%/co2ppm";
String MQTT_TOPIC_TEMP       = "%CHIP_ID%/tempC";
String MQTT_TOPIC_CO2ALARM   = "%CHIP_ID%/co2alarm";


void setupMqttTopic(const String &id)
{
  MQTT_TOPIC_STATE     .replace("%CHIP_ID%", id);
  MQTT_TOPIC_SETCONFIG .replace("%CHIP_ID%", id);
  MQTT_TOPIC_CO2PPM    .replace("%CHIP_ID%", id);
  MQTT_TOPIC_TEMP      .replace("%CHIP_ID%", id);
  MQTT_TOPIC_CO2ALARM  .replace("%CHIP_ID%", id);
}



// Callback when data is sent
void data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  lastEspNowSentStatus = (status == 0) ? true : false;
}



void setup()
{
  DEBUG_BEGIN(115200);

  DEBUG_PRINTLN("CO2 Sensor MHZ 19B");

  pinMode(OTA_BUTTON, INPUT_PULLUP);


  esp_efuse_mac_get_default(mac_addr);
  MQTTDEVICEID += String(mac_addr[4], HEX);
  MQTTDEVICEID += String(mac_addr[5], HEX);

  setupMqttTopic(MQTTDEVICEID);

  co2Serial.begin(9600); //Init sensor MH-Z19(14)


  // prepare rgb data pin
  pinMode(DATA_PIN, OUTPUT);
  digitalWrite(DATA_PIN, 0);

  FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).
    setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);

  fillSolid(leds, 0, NUM_LEDS, CRGB::Black);
  FastLED.show();


  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  tft.println("wait for OTA mode...");
  delay(5000);
  tft.fillScreen(TFT_BLACK);


  if (LOW == digitalRead(OTA_BUTTON)) {
    // enable AP/OTA mode only if powered up while a key pressed
    WiFi.softAPdisconnect(true); // deconfigure and disable
    WiFi.enableAP(false); // disable per default

    tft.setTextFont(4);
    tft.setTextDatum(TC_DATUM);
    tft.println("entering OTA mode");

    WiFi.enableAP(true);
    if (WiFi.softAP(ssid, password)) {
      DEBUG_PRINTLN("AP Ready");
      tft.println("AP/OTA ready");
      OTA_ENABLED = true;
    }
    else {
      DEBUG_PRINTLN("Soft AP setup Failed!");
      tft.setTextColor(TFT_RED);
      tft.println("AP seup failed");
    }
  }
  else {
    isWifiAvailable = setupWifi() ? false : true;
    net.setCACert(server_crt_str);
    net.setCertificate(client_crt_str);
    net.setPrivateKey(client_key_str);
  }

  if (isWifiAvailable) {
    mqttClient.setServer(mqtt_host, mqtt_port);
    mqttClient.setCallback(mqttCallback);
    isMqttAvailable = mqttConnect() ? false : true;
  }
  if (isMqttAvailable) {
    tft.fillScreen(TFT_BLACK);
  }


  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.onStart([]() {
		       String type;
		       if (ArduinoOTA.getCommand() == U_FLASH) {
			 type = "sketch";
		       } else { // U_FS
			 type = "filesystem";
		       }
		       // NOTE: if updating FS this would be the place to unmount FS using FS.end()
		       DEBUG_PRINTLN("Start updating " + type);
		       tft.fillScreen(TFT_BLACK);
		       fillSolid(leds, 0, NUM_LEDS, CRGB::Blue);
		     });
  ArduinoOTA.onEnd([]() {
		     DEBUG_PRINTLN("\nEnd");
		   });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
			  unsigned int percent = progress / (total / 100);
			  unsigned dot = ((unsigned long)(percent * NUM_LEDS/100));

			  if (progress < 1) tft.fillScreen(TFT_BLACK);
			  tft.setTextColor(TFT_BLUE);
			  tft.setTextFont(4);
			  tft.setCursor(20, 40);
			  tft.println("OTA PROGRESS");
			  tft.fillRect(3, 60, 100, 30, TFT_BLACK);
			  tft.setCursor(30, 60); // println without setcursor is ok!
			  tft.println(percent);
			  //tft.println(dot);
			  //fillSolid(leds, 0, dot, CRGB::White);
			  leds[(NUM_LEDS - 1 - dot)] = CRGB::White;
			  FastLED.show();
			});
  ArduinoOTA.onError([](ota_error_t error) {
		       tft.fillScreen(TFT_BLACK);
		       tft.setTextColor(TFT_WHITE);
		       tft.setTextFont(2);
		       tft.println("***  OTA ERROR !!!");
		       delay(1000);

		       if (error == OTA_AUTH_ERROR) {
			 DEBUG_PRINTLN("Auth Failed");
			 tft.println("***  AUTH !!!");
		       } else if (error == OTA_BEGIN_ERROR) {
			 DEBUG_PRINTLN("Begin Failed");
			 tft.println("***  BEGIN failed !!!");
		       } else if (error == OTA_CONNECT_ERROR) {
			 DEBUG_PRINTLN("Connect Failed");
			 tft.println("***  connect failed !!!");
		       } else if (error == OTA_RECEIVE_ERROR) {
			 DEBUG_PRINTLN("Receive Failed");
			 tft.println("***  receive failed !!!");
		       } else if (error == OTA_END_ERROR) {
			 DEBUG_PRINTLN("End Failed");
			 tft.println("***  END failed !!!");
		       }
		       delay(5000);
		     });
  ArduinoOTA.begin();
//}

  isMqttAvailable = mqttClient.publish(MQTT_TOPIC_STATE.c_str(),
				       "ESP32 TTGO CO2 Ampel Starting", true);
  /*
    ADC_EN is the ADC detection enable port
    If the USB port is used for power supply, it is turned on by default.
    If it is powered by battery, it needs to be set to high level
  */
  // could use light level to auto adjust led brightness
  // pinMode(ADC_EN, OUTPUT);
  // digitalWrite(ADC_EN, HIGH);
  // uint16_t v = analogRead(ADC_PIN); // could read ambient light via ldr?
				    // ... or use i2c light sensor

  // TODO: check if adc enable cause the issue with pullup???
  // setup globals


  if (true == OTA_ENABLED) return;

  // if we do not have an accespoint connection init esp_now network
  // transmit data to mac address of our office esp
  if ((! isWifiAvailable) && (! OTA_ENABLED)) {
    WiFi.mode(WIFI_STA);
    tft.println("init esp_now");
    if (esp_now_init() != ESP_OK) {
      DEBUG_PRINTLN("Error initializing ESP-NOW");
      return;
    }
    esp_now_register_send_cb(data_sent);

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      DEBUG_PRINTLN("Failed to add peer");
      return;
    }
  }



  tft.fillScreen(TFT_BLACK);
  tft.setCursor(5, 0);
  tft.println("ESP TTGO CO2 Ampel");
  tft.println("");
  tft.setTextColor(TFT_RED);
  tft.setTextFont(4);
  tft.setTextDatum(TC_DATUM);

  DEBUG_PRINT("Preheating");
  tft.println("Preheating "); // todo: progress bar
  tft.println("");
  drawVersion();

  tft.setTextColor(TFT_RED);
  tft.setTextFont(4);
  ledSpin(MHZ19B_PREHEATING_TIMEs);

  // start green
  fillSolid(leds, 0, NUM_LEDS, CRGB::Green);
  FastLED.show();

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(5, 0);
  tft.println("ESP TTGO CO2 Ampel");
  drawVersion();

  DEBUG_PRINTLN("");
}

void loop()
{

  if (isWifiAvailable || (true == OTA_ENABLED))
    ArduinoOTA.handle();

  if (isWifiAvailable && (false == (isMqttAvailable = mqttClient.loop()))) {
    DEBUG_PRINTLN("mqtt connection lost ... try re-connect");
    isMqttAvailable = mqttConnect(false) ? false : true;
  }

  if ((millis() - sw_timer_clock) > (5 * EVERY_SECOND)) {
    sw_timer_clock = millis();

    Serial.print("\n----- Time from start: ");
    Serial.print(millis() / 1000);
    Serial.println(" s");

    int rc = getCO2andTemp(data, 2);
    txReadings.co2  = data[0];
    txReadings.temp = data[1];
    if (rc) {
      DEBUG_PRINT("ERROR getting sensor data: rc=");
      DEBUG_PRINTLN(rc);
    }
    DEBUG_PRINT("PPM: ");
    DEBUG_PRINTLN(data[0]);

    DEBUG_PRINT("Temperature: ");
    DEBUG_PRINTLN(data[1]);

    ledShowStatus(data, 2);

    drawSensorData(data, 2);

    String co2ppm(data[0]);
    String tempC(data[1]);
    isMqttAvailable = mqttClient.publish(MQTT_TOPIC_CO2PPM.c_str(), co2ppm.c_str(), true);
    isMqttAvailable = mqttClient.publish(MQTT_TOPIC_TEMP.c_str(), tempC.c_str(), true);

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&txReadings, sizeof(txReadings));
    if (result == ESP_OK) {
      DEBUG_PRINTLN("Sent with success");
    }
    else {
      DEBUG_PRINTLN("Error sending the data");
    }

  }
}
