
String ipAddr;
String dnsAddr;
const unsigned maxWifiWaitSeconds = 60;
const int maxMqttRetry = 5;
bool mqttConnected = false;


//const char* mqttXXX      = "/" MQTTDEVICEID "/XXX";

// note: delays mainly to keep tft text shortly readable

int setupWifi() {
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("Connecting to wifi");

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);

  tft.setCursor(20, 60);
  tft.println("Connecting to WiFi");

  unsigned retry_counter = 0;
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    DEBUG_PRINT(".");
    tft.print(".");
    // display.drawXbm(46, 30, 8, 8, retry_counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    // display.drawXbm(60, 30, 8, 8, retry_counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    // display.drawXbm(74, 30, 8, 8, retry_counter % 3 == 2 ? activeSymbole : inactiveSymbole);

    retry_counter++;
    if (retry_counter > maxWifiWaitSeconds) {
      DEBUG_PRINTLN(" TIMEOUT!");
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(20, 60);
      tft.println("Wifi TIMEOUT");
      return 1;
    }
  }
  ipAddr  = WiFi.localIP().toString();
  dnsAddr = WiFi.dnsIP().toString();

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(ipAddr);
  DEBUG_PRINTLN("DNS address: ");
  DEBUG_PRINTLN(dnsAddr);

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 60);
  tft.println("Wifi CONNECTED");
  tft.print("IP Addr: "); tft.println(ipAddr);
  delay(3000);
  tft.fillScreen(TFT_BLACK);
  return 0;
}





/******************************************************************************
 *  MQTT
 */
int mqttConnect(bool updateDisplay)
{
  DEBUG_PRINT("Attempting MQTT connection...");
  String connect_msg = "CONNECTED ";
  int rc = 0;
  connect_msg += VERSION;

  if (updateDisplay) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(2); // 2, 4,

    tft.setCursor(20, 60);
    tft.println("Connecting to MQTT Server");
    delay(1000);
  }
  // Attempt to connect
  //mqttClient.setKeepAlive(0); // "fix" timeouts
  if (mqttClient.connect(MQTTDEVICEID.c_str(),
			 mqtt_user, mqtt_pass, MQTT_TOPIC_STATE.c_str(), 1, 1, "OFFLINE")) {
    DEBUG_PRINTLN("connected");
    if (updateDisplay) {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(20, 60);
      tft.println("MQTT CONNECTED");
    }
    // Once connected, publish an announcement...
    mqttClient.publish(MQTT_TOPIC_STATE.c_str(), connect_msg.c_str(), true);

    // topic subscriptions ...
    //mqttClient.subscribe(mqttSetMode);
    mqttClient.subscribe(MQTT_TOPIC_SETCONFIG.c_str());
    // mqttClient.subscribe();
  }
  else {
    DEBUG_PRINT("failed, mqttClient.state = ");
    DEBUG_PRINTLN(mqttClient.state());
    DEBUG_PRINT_MQTTSTATE(mqttClient.state());
    if (updateDisplay) {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(20, 60);
      tft.println("MQTT connection FAILED");
    }
    rc = 1;
  }
  delay(1000);
  if (updateDisplay) tft.fillScreen(TFT_BLACK);
  return rc;
}


void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  DEBUG_PRINT("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");

  char value[length+1];
  memcpy(value, payload, length);
  value[length] = '\0';

  DEBUG_PRINTLN(value);

  if (0 == strcmp(MQTT_TOPIC_SETCONFIG.c_str(), topic)) {
    DEBUG_PRINTLN("set config via mqtt");

  }


}

