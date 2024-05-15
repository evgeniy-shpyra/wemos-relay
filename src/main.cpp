#include <Arduino.h>
#include <ArduinoJson.h>

#include "button.h"
#include "led.h"
#include "storage.h"
#include "server.h"

// #include <ESP8266WiFi.h>
#include <PubSubClient.h>


#define SETTINGS_LED_PIN 14
#define WORK_LED_PIN 5

#define ON_LED 0
#define OFF_LED 4
#define AUTO_CHANGING_LED 15

#define RESET_BTN 2
#define RELAY 16

#define TOGGLE_STATUS_BTN 12
#define TOGGLE_AUTO_CHANGING_BTN 13

const char *AP_SSID = "WemosRelay";
const char *AP_PASS = "12345678";

String name;
String hubIp;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

Led settingsLed(SETTINGS_LED_PIN);
Led workLed(WORK_LED_PIN);
Led onLed(ON_LED);
Led offLed(OFF_LED);
Led autoChangingLed(AUTO_CHANGING_LED);

Button toggleStatusBtn(TOGGLE_STATUS_BTN);
Button toggleAutoChangingBtn(TOGGLE_AUTO_CHANGING_BTN);
Button resetBtn(RESET_BTN);

Storage storage;
SettingsServer settingsServer(storage, settingsLed);

bool currStatus = false;
bool isAutoToggled = false;
bool isAutoChanging = true;
bool statusBeforeAction = false;

bool isWorkMode = false;

void sendStatus()
{
  String topic = "device/" + name + "/status/set";
  String jsonString = "{\"status\": " + String(currStatus ? "true" : "false") + "}";

  mqttClient.publish(topic.c_str(), jsonString.c_str());
}

void toggleStatus(bool isOn)
{
  if (isOn)
  {
    digitalWrite(RELAY, HIGH);
    onLed.on();
    offLed.off();
    currStatus = true;
  }
  else
  {
    offLed.on();
    onLed.off();
    digitalWrite(RELAY, LOW);
    currStatus = false;
  }

  sendStatus();
}

void toggleAutoChanging(bool isOn)
{
  isAutoChanging = isOn;
  isOn ? autoChangingLed.on() : autoChangingLed.off();
}

void readButtons()
{
  toggleStatusBtn.loop();
  toggleAutoChangingBtn.loop();
  resetBtn.loop();

  if (toggleStatusBtn.isClick())
  {
    Serial.println("toggleStatusBtn");
    toggleStatus(!currStatus);
  }

  if (toggleAutoChangingBtn.isClick())
  {
    Serial.println("toggleAutoChangingBtn");
    toggleAutoChanging(!isAutoChanging);
  }

  if (resetBtn.isClick())
  {
    Serial.println("resetBtn");
    storage.deleteSettings();
    ESP.restart();
  }
}

void handleAutoChangeStatus(bool deviceStatus, bool sensorStatus)
{
  
  if (!isAutoChanging)
    return;
  if (sensorStatus)
  {
    isAutoToggled = true;
    statusBeforeAction = currStatus;
    toggleStatus(deviceStatus);
  }
  else
  {
    if (isAutoToggled)
    {
      toggleStatus(statusBeforeAction);
      isAutoToggled = false;
    }
  }
}

void MQTTcallback(char *topic, byte *payload, unsigned int length)
{
  String changeStatusTopic = "device/" + name + "/status/change";

  // Serial.print("Message received in topic: ");
  // Serial.println(topic);
  // Serial.print("Message:");
  String message;
  for (int i = 0; i < length; i++)
  {
    message = message + (char)payload[i];
  }

  // Serial.println(message);

  DynamicJsonDocument doc(200);
  DeserializationError error = deserializeJson(doc, message);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  if (strcmp(topic, "ping") == 0)
  {
    sendStatus();
  }
  else if (strcmp(topic, changeStatusTopic.c_str()) == 0)
  {
    Serial.println(message);
    if (doc["isAction"])
    {
      handleAutoChangeStatus(doc["deviceStatus"], doc["sensorStatus"]);
    }
    else
    {
      toggleStatus(doc["deviceStatus"]);
    }
  }
}

void tryMQTTConnect()
{
  mqttClient.setServer(hubIp.c_str(), 1883);
  while (!mqttClient.connected())
  {
    Serial.println("Connecting to MQTT");
    readButtons();

    String mqttName = "device-" + name;
    if (mqttClient.connect(name.c_str()))
    {
      Serial.println("connected");
      workLed.on();
    }
    else
    {
      workLed.toggle();
      Serial.print(".");
      delay(250);
    }
  }
  mqttClient.setCallback(MQTTcallback);
  mqttClient.subscribe("ping");

  String changeStatusTopic = "device/" + name + "/status/change";
  mqttClient.subscribe(changeStatusTopic.c_str());

  String getStatusTopic = "device/" + name + "/status/get";
  mqttClient.publish(getStatusTopic.c_str(), "true");
}

void setup()
{
  Serial.begin(115200);

  // leds
  workLed.setup();
  offLed.setup();
  onLed.setup();
  autoChangingLed.setup();

  pinMode(RELAY, OUTPUT);

  // buttons
  toggleStatusBtn.setup();
  toggleAutoChangingBtn.setup();
  resetBtn.setup();

  settingsServer.setup();
  bool isSettings = storage.settingsExist();

  toggleStatus(false);

  if (isSettings == true)
  {
    isWorkMode = true;
    toggleAutoChanging(true);
    SettingsStructure settings = storage.getSettings();

    name = settings.name;
    hubIp = settings.hubIp;

    WiFi.begin(settings.wifiSsid, settings.wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
      workLed.toggle();
      readButtons();
      Serial.print(".");
      delay(500);
    }
    workLed.off();
    Serial.print("Connected to WiFi :");
    Serial.println(WiFi.SSID());

    tryMQTTConnect();
  }
  else
  {
    toggleAutoChanging(false);
    Serial.print("data isn't exist");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    IPAddress IP = WiFi.softAPIP();

    Serial.print("Access Point IP address: ");
    Serial.println(IP);

    settingsServer.startServer();
  }
}

void loop()
{
  readButtons();

  if (isWorkMode)
  {
    if (!mqttClient.connected())
    {
      workLed.toggle();
      delay(250);
      workLed.toggle();
      delay(250);
      tryMQTTConnect();
    }
    mqttClient.loop();
  }
  else
  {
    settingsServer.loop();
  }
}
