#include <Arduino.h>
#include "storage.h"
#include "server.h"
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>

#define SETTINGS_LED_PIN 14
#define WORK_LED_PIN 5

#define ON_LED 0
#define OFF_LED 4

#define RESET_BTN 2
#define RELAY 16

#define ON_BTN 12
#define OFF_BTN 13

const char *AP_SSID = "WemosRelay";
const char *AP_PASS = "12345678";

WebSocketsClient webSocket;

bool currStatus = false;
bool isAutoToggled = false;
bool statusBeforeAction = false;
void turnOnRelay()
{
  digitalWrite(RELAY, HIGH);
  digitalWrite(ON_LED, HIGH);
  digitalWrite(OFF_LED, LOW);
  currStatus = true;
  webSocket.sendTXT("{\"action\": \"changeStatus\", \"status\": true}");
}
void turnOffRelay()
{
  digitalWrite(RELAY, LOW);
  digitalWrite(OFF_LED, HIGH);
  digitalWrite(ON_LED, LOW);
  currStatus = false;
  webSocket.sendTXT("{\"action\": \"changeStatus\", \"status\": false}");
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
      bool status = doc["status"];
      bool actionStatus = doc["actionStatus"];
      if (actionStatus)
      {
        isAutoToggled = true;
        statusBeforeAction = currStatus;
        status ? turnOnRelay() : turnOffRelay();
      }
      else
      {
        if (isAutoToggled)
        {
          statusBeforeAction ? turnOnRelay() : turnOffRelay();
          isAutoToggled = false;
        }
      }
    }
    else if(action == "getStatus"){
      StaticJsonDocument<50> jsonDocument;

      jsonDocument["action"] = "status";
      jsonDocument["status"] = currStatus;

      String jsonString;
      serializeJson(jsonDocument, jsonString);

      // Выводим JSON на Serial Monitor
      webSocket.sendTXT(jsonString);
    }
    break;
  }
}

Storage storage;
SettingsServer settingsServer(storage, SETTINGS_LED_PIN);

void readButtons()
{
  int resetBtn = digitalRead(RESET_BTN);
  if (resetBtn == LOW)
  {
    storage.deleteSettings();
    ESP.restart();
  }

  int onBtn = digitalRead(ON_BTN);
  int offBtn = digitalRead(OFF_BTN);
  
  if (onBtn == LOW && offBtn == LOW)
  {
    return;
  }
  if (onBtn == LOW && !currStatus)
  {
    turnOnRelay();
  }

  if (offBtn == LOW && currStatus)
  {
    turnOffRelay();
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(RESET_BTN, INPUT_PULLUP);
  pinMode(ON_BTN, INPUT_PULLUP); 
  pinMode(OFF_BTN, INPUT_PULLUP); 
  pinMode(WORK_LED_PIN, OUTPUT);
  pinMode(SETTINGS_LED_PIN, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(OFF_LED, OUTPUT);
  pinMode(ON_LED, OUTPUT);

  settingsServer.setup();
  bool isSettings = storage.settingsExist();

  turnOffRelay();

  if (isSettings == true)
  {
    SettingsStructure settings = storage.getSettings();
    Serial.println(settings.wifiSsid);
    Serial.println(settings.wifiPassword);
    Serial.println(settings.name);
    Serial.println(settings.key);

    WiFi.begin(settings.wifiSsid, settings.wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(WORK_LED_PIN, HIGH);
      delay(250);
      digitalWrite(WORK_LED_PIN, LOW);
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
