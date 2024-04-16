#ifndef SERVER_H
#define SERVER_H
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class SettingsServer
{
private:
  ESP8266WebServer server;

public:
  SettingsServer()
  {
  }

  void startServer()
  {
    server.on("/", HTTP_GET, [this]()
              { this->rootRoute(); });
    server.on("/submit", HTTP_POST, [this]()
              { this->submitRoute(); });

    server.begin();
    Serial.println("HTTP server started");
  }

  void stopServer()
  {
    server.stop();
  }

  void handleClient()
  {
    server.handleClient();
  }

private:
  String generateHtml()
  {
    String ssidSelect = "";
    int networkCount = WiFi.scanNetworks();
    for (int i = 0; i < networkCount; ++i)
    {
      String name = WiFi.SSID(i);
      ssidSelect += "<option value=\"" + name + "\">" + name + "</option>";
    }

    String html = "<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>Wemos led</title><style>html{height:100%}body{height:100%;display:flex;align-items:center;justify-content:center;font-family:Arial,Helvetica,sans-serif}div{width:230px}h2,input{color:#5d5fef}h2{text-align:center}input[type=password],input[type=text]{padding:10px 15px;border:2px #d9d9d9 solid;border-radius:8px;font-size:13px;margin-bottom:5px;width:100%;box-sizing:border-box;outline:0}label{font-size:13px;color:gray}input[type=password]:focus,input[type=text]:focus{border:2px #ababab solid}button{background-color:#5d5fef;color:#fff;width:100%;text-align:center;padding:10px 0;border:none;border-radius:5px;box-sizing:border-box}select{display:block;font-size:13px;font-family:sans-serif;color:#6e6c6c;width:100%;max-width:100%;box-sizing:border-box;margin:0;padding:10px 12px;border:2px #d9d9d9 solid;border-radius:8px;background-color:#fff}select option:disabled{color:gray}select:focus{border:2px #ababab solid;color:#222;outline:0}select option{font-weight:400}</style></head><body><div><h2>Settings</h2><form class='form' method='post' action='/submit'><input type='text' name='name' placeholder='Name'><br><select name='ssid'><option value='' disabled='disabled' selected='selected'>Select wifi name</option>" + ssidSelect + "</select><input style='margin-top:5px' type='password' name='wifiPassword' placeholder='Wifi password'><br><input type='text' name='brokerIp' placeholder='Broker ip'><br><input type='text' name='topic' placeholder='Topic'><br><button type='submit'>SAVE</button></form></div><script>document.getElementsByName('ssid')[0].addEventListener('change',function(){this.style.color='#5d5fef'})</script></body></html>";
    return html;
  }

  void rootRoute()
  {
    server.send(200, "text/html", generateHtml());
  }

  void submitRoute()
  {
    // Обработка данных формы
    // String name = server.arg("name");
    // String wifiSsid = server.arg("ssid");
    // String wifiPassword = server.arg("wifiPassword");
    // String brokerIp = server.arg("brokerIp");
    // String topic = server.arg("topic");

    // Settings newSettings;

    // name.toCharArray(newSettings.name, sizeof(newSettings.name));
    // wifiSsid.toCharArray(newSettings.wifiSsid, sizeof(newSettings.wifiSsid));
    // wifiPassword.toCharArray(newSettings.wifiPassword, sizeof(newSettings.wifiPassword));
    // brokerIp.toCharArray(newSettings.brokerIp, sizeof(newSettings.brokerIp));
    // topic.toCharArray(newSettings.topic, sizeof(newSettings.topic));
    // newSettings.isValid = true;

    // updateSettings(newSettings);

    server.send(200, "text/plain", "Data submitted successfully!");
  }
};

#endif // SERVER_H