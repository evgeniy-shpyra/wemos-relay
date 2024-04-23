#include <Arduino.h>

#define SETTINGS_LED_PIN 4
#define WORK_LED_PIN 0
#define RESET_BTN_PIN 2
#define RELAY_PIN 16

#include "storage.h"
#include "server.h"
#include <WebSocketsClient.h>
#include <Hash.h>
#include <ArduinoJson.h>

const char *AP_SSID = "Wemos-led";
const char *AP_PASS = "12345678";

WebSocketsClient webSocket;

void prepareJson(String &str)
{
  char c = '\\';
  str.remove(0, 1);
  str.remove(str.length() - 1);
  while (str.indexOf(c) != -1)
  {
    str.replace(String(c), "");
  }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
  {
    Serial.println("[WSc] Connected to url:");
    Serial.println(String((char *)payload));
    webSocket.sendTXT("Connected");
  }
  break;
  case WStype_TEXT:
    String dataJson = String((char *)payload);
    prepareJson(dataJson);
    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, dataJson);
    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    bool status = doc["status"];
    String action = doc["action"];

    if (action == "toggleStatus")
    {
      if (status)
      {
        Serial.println("on");
         digitalWrite(RELAY_PIN, HIGH);
      }
      else
      {
        Serial.println("off");
         digitalWrite(RELAY_PIN, LOW);
      }
    }
    break;
  }
}

Storage storage;
SettingsServer settingsServer(storage, SETTINGS_LED_PIN);

void readResetBtn()
{
  int resetBtn = digitalRead(RESET_BTN_PIN);
  if (resetBtn == LOW)
  {
    Serial.println("Button is pressed");
    storage.deleteSettings();
    ESP.restart();
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(RESET_BTN_PIN, INPUT);
  pinMode(WORK_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  settingsServer.setup();
  bool isSettings = storage.settingsExist();

  if (isSettings == true)
  {
    SettingsStructure settings = storage.getSettings();
    Serial.println(settings.wifiSsid);
    Serial.println(settings.wifiPassword);

    WiFi.begin(settings.wifiSsid, settings.wifiPassword);
    while (WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(WORK_LED_PIN, HIGH);
      delay(250);
      digitalWrite(WORK_LED_PIN, LOW);
      delay(250);
      readResetBtn();
      Serial.print(".");
    }
    digitalWrite(WORK_LED_PIN, HIGH);
    Serial.println("Connected");
    Serial.println(WiFi.localIP());

    webSocket.begin(settings.hubIp, 9000, "/ws/device");
    webSocket.setAuthorization("1", "111");
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
  readResetBtn();
  settingsServer.loop();
  webSocket.loop();
}
