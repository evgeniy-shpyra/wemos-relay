#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define STR_ADDR 0
const char* AP_SSID = "Wemos-led";
ESP8266WebServer server(80);


struct Settings {
  bool isValid;
  char name[50];
  char brokerIp[16];
  char topic[50];
  char wifiSsid[32];
  char wifiPassword[32];
};

bool getSettings(const Settings& data) {
  EEPROM.begin(sizeof(Settings));
  EEPROM.get(STR_ADDR, data);
  EEPROM.end();

  return data.isValid;
}

void updateSettings(const Settings& data) {
  EEPROM.begin(sizeof(Settings));
  EEPROM.put(STR_ADDR, data);
  EEPROM.commit();
  EEPROM.end();
}

void deleteSettings() {
    Settings emptyData;
    memset(&emptyData, 0, sizeof(emptyData));
    emptyData.isValid = false;
    updateSettings(emptyData);
}

const char* htmlForm = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Wemos led</title><style>html{height:100%}body{height:100%;display:flex;align-items:center;justify-content:center;font-family:Arial,Helvetica,sans-serif}div{width:230px}h2,input{color:#5d5fef}h2{text-align:center}input[type=password],input[type=text]{padding:10px 15px;border:2px #d9d9d9 solid;border-radius:8px;font-size:13px;margin-bottom:5px;width:100%;box-sizing:border-box;outline:0}input[type=password]:focus,input[type=text]:focus{border:2px #ababab solid}button{background-color:#5d5fef;color:#fff;width:100%;text-align:center;padding:10px 0;border:none;border-radius:5px;box-sizing:border-box}</style></head><body><div><h2>Settings</h2><form class='form' method='post' action='/submit'><input type='text' name='name' placeholder='Name'><br><input type='text' name='wifiSsid' placeholder='Wifi ssid'><br><input type='password' name='wifiPassword' placeholder='Wifi password'><br><input type='text' name='brokerIp' placeholder='Broker ip'><br><input type='text' name='topic' placeholder='Topic'><br><button type='submit'>SAVE</button></form></div></body></html>";


void handleRoot() {
  server.send(200, "text/html", htmlForm);
}


void handleSubmit() {
  // Обработка данных формы
  String name = server.arg("name");
  String wifiSsid = server.arg("wifiSsid");
  String wifiPassword = server.arg("wifiPassword");
  String brokerIp = server.arg("brokerIp");
  String topic = server.arg("topic");

  Settings newSettings;

  name.toCharArray(newSettings.name, sizeof(newSettings.name));
  wifiSsid.toCharArray(newSettings.wifiSsid, sizeof(newSettings.wifiSsid));
  wifiPassword.toCharArray(newSettings.wifiPassword, sizeof(newSettings.wifiPassword));
  brokerIp.toCharArray(newSettings.brokerIp, sizeof(newSettings.brokerIp));
  topic.toCharArray(newSettings.topic, sizeof(newSettings.topic));
  newSettings.isValid = true;

  updateSettings(newSettings);

  server.send(200, "text/plain", "Data submitted successfully!");
}



void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);

  IPAddress IP = WiFi.softAPIP();

  Serial.print("Access Point IP address: ");
  Serial.println(IP);
  
  // Настройка маршрутов HTTP
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  
  server.begin();
  Serial.println("HTTP server started");

  // deleteSettings();

  Settings settings;
  
  bool isExist = getSettings(settings);

  if(isExist){
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
  else {
    Serial.print("data isn't exist");
  }
}

void loop() {
 server.handleClient(); // Обработка клиентских запросов
}
