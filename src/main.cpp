#include <Arduino.h>
#include "storage.h"
#include "server.h"
#include "button.h"
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>

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

WebSocketsClient webSocket;

Button toggleStatusBtn(TOGGLE_STATUS_BTN);
Button toggleAutoChangingBtn(TOGGLE_AUTO_CHANGING_BTN);
Button resetBtn(RESET_BTN);

bool currStatus = false;
bool isAutoToggled = false;
bool isAutoChanging = true;
bool statusBeforeAction = false;

void toggleStatus(bool isOn)
{
  if (isOn)
  {
    digitalWrite(RELAY, HIGH);
    digitalWrite(ON_LED, HIGH);
    digitalWrite(OFF_LED, LOW);
    currStatus = true;
    webSocket.sendTXT("{\"action\": \"changeStatus\", \"status\": true}");
  }
  else
  {
    digitalWrite(RELAY, LOW);
    digitalWrite(OFF_LED, HIGH);
    digitalWrite(ON_LED, LOW);
    currStatus = false;
    webSocket.sendTXT("{\"action\": \"changeStatus\", \"status\": false}");
  }
}

void toggleAutoChanging(bool isOn)
{
  isAutoChanging = isOn;
  digitalWrite(AUTO_CHANGING_LED, isOn ? HIGH : LOW);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("[WSc] Disconnected!\n");
    digitalWrite(WORK_LED_PIN, LOW);
    break;
  case WStype_CONNECTED:
  {
    Serial.println("[WSc] Connected to url:");
    Serial.println(String((char *)payload));
    digitalWrite(WORK_LED_PIN, HIGH);
  }
  break;
  case WStype_TEXT:
    String dataJson = String((char *)payload);
    Serial.println(dataJson);
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, dataJson);
    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    String action = doc["action"];

    if (action == "autoToggleStatus")
    {
      if(!isAutoChanging) return;
      bool status = doc["status"];
      bool actionStatus = doc["actionStatus"];
      if (actionStatus)
      {
        isAutoToggled = true;
        statusBeforeAction = currStatus;
        toggleStatus(status);
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
    else if (action == "getStatus")
    {
      StaticJsonDocument<50> jsonDocument;

      jsonDocument["action"] = "status";
      jsonDocument["status"] = currStatus;

      String jsonString;
      serializeJson(jsonDocument, jsonString);

      webSocket.sendTXT(jsonString);
    }
    else if(action == "changeStatus"){
      bool status = doc["status"];
      toggleStatus(status);
    }
    break;
  }
}

Storage storage;
SettingsServer settingsServer(storage, SETTINGS_LED_PIN);

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

void setup()
{
  Serial.begin(115200);

  pinMode(WORK_LED_PIN, OUTPUT);
  pinMode(SETTINGS_LED_PIN, OUTPUT);
  pinMode(AUTO_CHANGING_LED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(OFF_LED, OUTPUT);
  pinMode(ON_LED, OUTPUT);

  toggleStatusBtn.setup();
  toggleAutoChangingBtn.setup();
  resetBtn.setup();

  settingsServer.setup();
  bool isSettings = storage.settingsExist();

  toggleStatus(false);
  

  if (isSettings == true)
  {
    toggleAutoChanging(true);
    SettingsStructure settings = storage.getSettings();

    WiFi.begin(settings.wifiSsid, settings.wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(WORK_LED_PIN, HIGH);
      readButtons();
      delay(250);
      digitalWrite(WORK_LED_PIN, LOW);
      readButtons();
      delay(250);
      readButtons();
      Serial.print(".");
    }
    digitalWrite(WORK_LED_PIN, LOW);
    Serial.println("Connected");
    Serial.println(WiFi.localIP());

    webSocket.begin(settings.hubIp, 9000, "/ws/device");
    webSocket.setAuthorization(settings.name, settings.key);
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    webSocket.enableHeartbeat(15000, 3000, 2);
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
  settingsServer.loop();
  webSocket.loop();
}
