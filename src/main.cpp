/* 
************************************************************************************************************************
* @file 
* @author
* @version
* @date
* @brief
************************************************************************************************************************
*/

// IP De configuração: 192.168.4.1

/* Includes  -------------------------------------------------------------------------------------------------------*/
#include <Arduino.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"

/* Constants --------------------------------------------------------------------------------------------------------*/

/* Pin Numbers ------------------------------------------------------------------------------------------------------*/
#define TOUCH_PIN 13

/* Constants --------------------------------------------------------------------------------------------------------*/

/* Private Variables ------------------------------------------------------------------------------------------------*/
const char* input_parameter1 = "ssid";
const char* input_parameter2 = "pass";
const char* input_parameter3 = "ip";

String ssid;
String pass;
String ip;

const char* SSID_path = "/ssid.txt";
const char* Password_path = "/pass.txt";
const char* IP_path = "/ip.txt";


unsigned long previous_time = 0;
const long Delay = 10000; 

IPAddress localIP;
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

/* Private Functions ------------------------------------------------------------------------------------------------*/
AsyncWebServer server(80);

String readFile(fs::FS &fs, const char * path);

void writeFile(fs::FS &fs, const char * path, const char * message);
void configureWiFi();
void printAPIP(void *pvParameters);

bool initialize_Wifi();

/* Main Aplication --------------------------------------------------------------------------------------------------*/


void setup() {
  Serial.begin(115200);

  SPIFFS.begin(true);
  
  ssid = readFile(SPIFFS, SSID_path);
  pass = readFile(SPIFFS, Password_path);
  ip = readFile(SPIFFS, IP_path);

  if(!initialize_Wifi())
    configureWiFi();

  xTaskCreate(printAPIP, "printAPIP", 4096, NULL, 1, NULL);

}

void loop() {

  if(touchRead(TOUCH_PIN) == LOW)
    configureWiFi();
}

void printAPIP(void *pvParameters) {
  (void)pvParameters;

  while (1) {
    byte lastOctet = localIP[3];

    Serial.print("IP address: ");
    Serial.println(lastOctet);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}


void configureWiFi(){
  
  Serial.println("Setting Access Point");
  WiFi.softAP("CONFIG-DIR-PALMILHA", "biolab");

  // IPAddress IP = WiFi.softAPIP();
  // Serial.print("AP IP address: ");
  // Serial.println(IP); 

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/wifimanager.html", "text/html");
  });
  
  server.serveStatic("/", SPIFFS, "/");
  
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){

        if (p->name() == input_parameter1) {
          ssid = p->value().c_str();
          writeFile(SPIFFS, SSID_path, ssid.c_str());
        }

        if (p->name() == input_parameter2) {
          pass = p->value().c_str();
          writeFile(SPIFFS, Password_path, pass.c_str());
        }

        if (p->name() == input_parameter3) {
          ip = p->value().c_str();
          writeFile(SPIFFS, IP_path, ip.c_str());
        }
      }
    }
    request->send(200, "text/plain", "Success. ESP32 will now restart. Connect to your router and go to IP address: " + ip);
    delay(3000);
    ESP.restart();
  });
  server.begin();
}


String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}


void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}


bool initialize_Wifi() {

  if(ssid=="" || ip=="")
    return false;

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());

  if (!WiFi.config(localIP, gateway, subnet))
    return false;
  
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long current_time = millis();
  previous_time = current_time;

  while(WiFi.status() != WL_CONNECTED) {
    current_time = millis();
    if (current_time - previous_time >= Delay)
      return false;
  }

  return true;
}
