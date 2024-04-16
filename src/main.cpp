#include <Arduino.h>
#include "server.h"

#include <EEPROM.h>

#define STR_ADDR 0
const char *AP_SSID = "Wemos-led";

SettingsServer settingsServer;

struct Settings
{
  bool isValid;
  char name[50];
  char brokerIp[16];
  char topic[50];
  char wifiSsid[32];
  char wifiPassword[32];
};

bool getSettings(const Settings &data)
{
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(STR_ADDR, data);
  EEPROM.end();

  return data.isValid;
}

void updateSettings(const Settings &data)
{
  EEPROM.begin(sizeof(Settings));
  EEPROM.put(STR_ADDR, data);
  EEPROM.commit();
  EEPROM.end();
}

void deleteSettings()
{
  Settings emptyData;
  memset(&emptyData, 0, sizeof(emptyData));
  emptyData.isValid = false;
  updateSettings(emptyData);
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  IPAddress IP = WiFi.softAPIP();

  Serial.print("Access Point IP address: ");
  Serial.println(IP);

  settingsServer.startServer();

  Settings settings;

  bool isExist = getSettings(settings);

  if (isExist == true)
  {
    Serial.print("data exist");
    Serial.print(settings.name);
    Serial.print("\n");
    Serial.print(settings.wifiSsid);
    Serial.print("\n");
    Serial.print(settings.wifiPassword);
    Serial.print("\n");
    Serial.print(settings.brokerIp);
    Serial.print("\n");
    Serial.print(settings.topic);
  }
  else
  {
    Serial.print("data isn't exist");
  }
}

void loop()
{
  settingsServer.handleClient(); // Обработка клиентских запросов
}
