/***************************************************************************
  soyosource web-limiter v1.0
  @matlen67 - 24.04.2023

  Wiring
  NodeMCU D1 - RS485 RO
  NodeMCU D3 - RS485 DE/RE
  NodeMCU D4 - RS485 DI

****************************************************************************/
#include <FS.h>  
#include "Arduino.h"
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsync_WiFiManager.h>
#include <ESPAsyncWebServer.h>

#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include "html.h"
//#include "favicon.h"
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>
#include <Uptime.h>
#include <time.h>

#define DEBUG false

#define DEBUG_SERIAL \
  if (DEBUG) Serial


#define RXPin        D1  // Serial Receive pin (D1)
#define TXPin        D4  // Serial Transmit pin (D4)
 
//RS485 control
#define SERIAL_COMMUNICATION_CONTROL_PIN D3 // Transmission set pin (D3)
#define RS485_TX_PIN_VALUE HIGH
#define RS485_RX_PIN_VALUE LOW

#define MY_NTP_SERVER "de.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"   
 
SoftwareSerial RS485Serial(RXPin, TXPin); // RX, TX

WiFiClient espClient;
PubSubClient client(espClient);


// Uptime Global Variables
Uptime uptime;
uint8_t Uptime_Years = 0U, Uptime_Months = 0U, Uptime_Days = 0U, Uptime_Hours = 0U, Uptime_Minutes = 0U, Uptime_Seconds = 0U;
uint16_t Uptime_TotalDays = 0U; // Total Uptime Days
char Uptime_Str[37];  

//Timer
unsigned long lastTime1 = 0;  
unsigned long timerDelay1 = 950;  // send readings timer


String msg = "";
char msgData[64];

//mqtt
char mqtt_server[16] = "192.168.178.10";
char mqtt_port[5] = "1889";

//default custom static IP
char static_ip[16] = "192.168.178.250";
char static_gw[16] = "192.168.178.1";
char static_sn[16] = "255.255.255.0";


String dataReceived;
int data;
bool isDataReceived = false;
uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7; 
int byteSend;
int data_array[8];
int soyo_hello_data[8] = {0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // bit7 org 0x00, CRC 0xFF
int soyo_power_data[8] = {0x24, 0x56, 0x00, 0x21, 0x00, 0x00, 0x80, 0x08}; // 0 Watt

int soyo_text_data[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int cur_watt;
int cur_power;

bool newData = false;
int value_power = 0;
unsigned char mac[6];


char mqtt_root[32] = "SoyoSource/";
char clientId[12];

char topic_alive[40];
char topic_power[40];
char soyo_text[40];

long rssi;

time_t now;                       
tm tm;

char currentTime[20];
char time_1[6] = "06:00";
char watt_1[6] = "0";
char time_2[6] = "20:00";
char watt_2[6] = "0";

bool checkboxT1 = false;
bool checkboxT2 = false;

String checkbox1 = "<input type='checkbox' onchange='toggleCheckbox(this)' id='cb1'/>";
String checkbox2 = "<input type='checkbox' onchange='toggleCheckbox(this)' id='cb2'/>";

String time1 = "<input type='time' id='t1' value='" + String(time_1) + "'/>";
String time2 = "<input type='time' id='t2' value='" + String(time_2) + "'/>";

String watt1 = "<input type='number' min='0' max='2000' value='" + String(watt_1) + "' id='w1'/>";
String watt2 = "<input type='number' min='0' max='2000' value='" + String(watt_2) + "' id='w2'/>";





AsyncWebServer server(80);
AsyncEventSource events("/events");
AsyncDNSServer dns;

const char* PARAM_MESSAGE = "message";


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  DEBUG_SERIAL.println("Should save config");
  shouldSaveConfig = true;
}


void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}


String processor(const String& var){
   
   DEBUG_SERIAL.println(var);

    //STATICPOWER
    if(var == "STATICPOWER"){
      return String(value_power);
    }
    else if(var == "WIFIRSSI"){
      return String(rssi);      
    }
    else if(var == "CLIENTID"){
      return String(clientId);
    }
    else if(var == "UPTIME"){
      myUptime();
      return Uptime_Str;
    }
    else if(var == "SOYOTEXT"){
      return soyo_text;
    }
    else if(var == "MQTTROOT"){
      return mqtt_root;
    }
    else if(var == "CHECKBOX1"){
      return checkbox1;
    }
    else if(var == "CHECKBOX2"){
      return checkbox2;
    }
    else if(var == "TIME1"){
      return time1;
    }
    else if(var == "TIME2"){
      return time2;
    }
    else if(var == "WATT1"){
      return watt1;
    }
    else if(var == "WATT2"){
      return watt2;
    }
    
    
    return String();
}


void reconnect() {
  while (!client.connected()) {
    DEBUG_SERIAL.println("Attempting MQTT connection...");
   
    if (client.connect(clientId)) {
      DEBUG_SERIAL.println("reconnect: connected");
      client.publish(topic_alive, "1");
      client.publish(topic_power, "0"); //set power 0 watt
      client.subscribe(topic_power);    // subscripe topic
    } else {
      DEBUG_SERIAL.print("reconnect: error! ");
      delay(5000);
    }
  }
}


//callback from mqtt
void callback(char* topic, byte* payload, unsigned int length) {
  DEBUG_SERIAL.print("Message arrived: ");
  DEBUG_SERIAL.print(topic);
  DEBUG_SERIAL.println(" ");
  bool isNumber = true;
  

  if(String(topic) == topic_power){

    for (int i=0;i<length;i++) {
      DEBUG_SERIAL.print((char)payload[i]);
    
      if(!isDigit(payload[i])){
        isNumber = false;
      }
    }
    
    if(isNumber && !newData){
      payload[length] = '\0'; 
      int aNumber = atoi((char *)payload);
      value_power = aNumber;
      newData = true;    
    }
  }

  DEBUG_SERIAL.println();
}


void setSoyoPowerData(int power){
  soyo_power_data[0] = 0x24;
  soyo_power_data[1] = 0x56;
  soyo_power_data[2] = 0x00;
  soyo_power_data[3] = 0x21;
  soyo_power_data[4] = power >> 0x08;
  soyo_power_data[5] = power & 0xFF;
  soyo_power_data[6] = 0x80;
  soyo_power_data[7] = calc_checksumme(soyo_power_data[1], soyo_power_data[2], soyo_power_data[3], soyo_power_data[4], soyo_power_data[5], soyo_power_data[6]);
}


int calc_checksumme(int b1, int b2, int b3, int b4, int b5, int b6 ){
  int calc = (0xFF - b1 - b2 - b3 - b4 - b5 -b6) % 256;
  return calc & 0xFF;
}


void myUptime(){
  uptime.calculateUptime();                                  

  // Get The Uptime Values To Global Variables
  Uptime_Years      = uptime.getYears();
  Uptime_Months     = uptime.getMonths();
  Uptime_Days       = uptime.getDays();
  Uptime_Hours      = uptime.getHours();
  Uptime_Minutes    = uptime.getMinutes();
  Uptime_Seconds    = uptime.getSeconds();
  Uptime_TotalDays  = uptime.getTotalDays();

  if (Uptime_Years == 0U) {                                  // Uptime Is Less Than One Year
    // First 60 Seconds
    if (Uptime_Minutes == 0U && Uptime_Hours == 0U && Uptime_Days == 0U && Uptime_Months == 0U)
      sprintf(Uptime_Str, "00:00:%02i", Uptime_Seconds);
    // First Minute
    else if (Uptime_Minutes == 1U && Uptime_Hours == 0U && Uptime_Days == 0U && Uptime_Months == 0U)
      sprintf(Uptime_Str, "00:%02i:%02i", Uptime_Minutes, Uptime_Seconds);
    // Second Minute And More But Less Than Hours, Days, Months
    else if (Uptime_Minutes >= 2U && Uptime_Hours == 0U && Uptime_Days == 0U && Uptime_Months == 0U)
      sprintf(Uptime_Str, "00:%02i:%02i", Uptime_Minutes, Uptime_Seconds);
    // First Hour And More But Less Than Days, Months
    else if (Uptime_Hours >= 1U && Uptime_Days == 0U && Uptime_Months == 0U)
      sprintf(Uptime_Str, "%02i:%02i:%02i", Uptime_Hours, Uptime_Minutes, Uptime_Seconds);
    // First Day And Less Than Month
    else if (Uptime_Days == 1U && Uptime_Months == 0U)
      sprintf(Uptime_Str, "%iday %02i:%02i:%02i", Uptime_Days, Uptime_Hours, Uptime_Minutes, Uptime_Seconds);
    // Second Day And More But Less Than Month
    else if (Uptime_Days >= 2U && Uptime_Months == 0U)
      sprintf(Uptime_Str, "%idays %02i:%02i:%02i", Uptime_Days, Uptime_Hours, Uptime_Minutes, Uptime_Seconds);
    // First Month And More But Less Than One Year
    else if (Uptime_Months >= 1U)
      sprintf(Uptime_Str, "%im, %id %02i:%02i", Uptime_Months, Uptime_Days, Uptime_Hours, Uptime_Minutes);
    // If There Is Any Error In This If Loop Then Make Full String.
    else sprintf(Uptime_Str, "%iy %im %id %02i:%02i", Uptime_Years, Uptime_Months, Uptime_Days, Uptime_Hours, Uptime_Minutes);
  } else                                                     // Uptime Is More Than One Year
    sprintf(Uptime_Str, "%iy %im %id %02i:%02i", Uptime_Years, Uptime_Months, Uptime_Days, Uptime_Hours, Uptime_Minutes);

}



void createTimeString(){
  time(&now);                       // read the current time
  localtime_r(&now, &tm);  

  sprintf(currentTime, "%d.%d.%d %02d:%02d:%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec );
  
  DEBUG_SERIAL.print("Datum Uhrzeit: ");
  DEBUG_SERIAL.println(currentTime);
}

void saveConfig(){

  DEBUG_SERIAL.println("saving config");
  #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
  #else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
  #endif
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;

  json["ip"] = WiFi.localIP().toString();
  json["gateway"] = WiFi.gatewayIP().toString();
  json["subnet"] = WiFi.subnetMask().toString();
  json["time1"] = time_1;
  json["watt1"] = watt_1;
  json["time2"] = time_2;
  json["watt2"] = watt_2;
  
  
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    DEBUG_SERIAL.println("failed to open config file for writing");
  }

  #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
  #else
    json.printTo(Serial);
    json.printTo(configFile);
  #endif
  configFile.close();

}


void setup()  {
  DEBUG_SERIAL.begin(115200);
  delay(250);

  WiFi.macAddress(mac);
  
  DEBUG_SERIAL.println("Start");
  DEBUG_SERIAL.printf("ESP_%02X%02X%02X", mac[3], mac[4], mac[5]);
  DEBUG_SERIAL.println();

  configTime(MY_TZ, MY_NTP_SERVER);
  
  //generate espid in hex like soyo_18d88d
 
  //clientId = "soyoxxxxxx";
  sprintf(clientId, "soyo_%02x%02x%02x", mac[3], mac[4], mac[5] );
  

  //mqtt_root = "SoyoSource/soyo_xxxxxx";
  strcat(mqtt_root, clientId);
  
  //topic_alive = "SoyoSource/soyo_xxxxxx/alive";
  strcat(topic_alive, mqtt_root);
  strcat(topic_alive, "/alive");

  //topic_power = "SoyoSource/soyo_xxxxxx/power";
  strcat(topic_power, mqtt_root);
  strcat(topic_power, "/power");

  DEBUG_SERIAL.println(topic_alive);
  
  pinMode(SERIAL_COMMUNICATION_CONTROL_PIN, OUTPUT);
  digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_RX_PIN_VALUE);
  RS485Serial.begin(4800);   // set RS485 baud

  //read configuration from FS json
  DEBUG_SERIAL.println("mounting FS...");

  if (SPIFFS.begin()) {
    DEBUG_SERIAL.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      DEBUG_SERIAL.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        DEBUG_SERIAL.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

        #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
          DEBUG_SERIAL.println("ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6");
          DynamicJsonDocument json(1024);
          auto deserializeError = deserializeJson(json, buf.get());
          serializeJson(json, Serial);
          if ( ! deserializeError ) {
        #else
          DynamicJsonBuffer jsonBuffer;
          JsonObject& json = jsonBuffer.parseObject(buf.get());
          json.printTo(Serial);
          if (json.success()) {
        #endif
          DEBUG_SERIAL.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);

          if(json.containsKey("time1")){
            DEBUG_SERIAL.println("time1 exist");
            strcpy(time_1, json["time1"]);
            time1 = "<input type='time' id='t1' value='" + String(time_1) + "'/>"; 
          }

          if(json.containsKey("time2")){
            DEBUG_SERIAL.println("time2 exist");
            strcpy(time_2, json["time2"]);
            time2 = "<input type='time' id='t2' value='" + String(time_2) + "'/>"; 
          }

          if(json.containsKey("watt1")){
            DEBUG_SERIAL.println("watt1 exist");
            strcpy(watt_1, json["watt1"]);
            watt1 = "<input type='number' min='0' max='2000' value='" + String(watt_1) + "' id='w1'/>";
          }

          if(json.containsKey("watt2")){
            DEBUG_SERIAL.println("watt2 exist");
            strcpy(watt_2, json["watt2"]);
            watt2 = "<input type='number' min='0' max='2000' value='" + String(watt_2) + "' id='w2'/>";
          }

        } else {
          DEBUG_SERIAL.println("failed to load json config");
        }
      }
    }
  } else {
    DEBUG_SERIAL.println("failed to mount FS");
  }
  //end read

  
  WiFi.persistent(true); // sonst verlieret er nach einem Neustart die IP !!!

  ESPAsync_WMParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  ESPAsync_WMParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5); 

  ESPAsync_WiFiManager wifiManager(&server,&dns);

  
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.setConfigPortalTimeout(180);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);

  String apName = "soyo_esp_" + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
  bool res;
  res = wifiManager.autoConnect(apName.c_str());

  if(!res) {
    DEBUG_SERIAL.println("Failed to connect");
    ESP.restart();
  }
  else {
    //if you get here you have connected to the WiFi    
    DEBUG_SERIAL.print("WiFi connected to ");
    DEBUG_SERIAL.println(String(WiFi.SSID()));
    DEBUG_SERIAL.print("RSSI = ");
    DEBUG_SERIAL.print(String(WiFi.RSSI()));
    DEBUG_SERIAL.println(" dB");
    DEBUG_SERIAL.print("IP address  ");
    DEBUG_SERIAL.println(WiFi.localIP());
    DEBUG_SERIAL.println();

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    
    //save the custom parameters to FS
    if (shouldSaveConfig) {
      saveConfig();
      // DEBUG_SERIAL.println("saving config");
      // #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
      //  DynamicJsonDocument json(1024);
      // #else
      //   DynamicJsonBuffer jsonBuffer;
      //   JsonObject& json = jsonBuffer.createObject();
      // #endif
      // json["mqtt_server"] = mqtt_server;
      // json["mqtt_port"] = mqtt_port;
  
      // json["ip"] = WiFi.localIP().toString();
      // json["gateway"] = WiFi.gatewayIP().toString();
      // json["subnet"] = WiFi.subnetMask().toString();

      // File configFile = SPIFFS.open("/config.json", "w");
      // if (!configFile) {
      //   DEBUG_SERIAL.println("failed to open config file for writing");
      // }

      // #if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
      //   serializeJson(json, Serial);
      //   serializeJson(json, configFile);
      // #else
      //   json.printTo(Serial);
      //   json.printTo(configFile);
      // #endif
      // configFile.close();
    }

    DEBUG_SERIAL.println("set mqtt server!");
    DEBUG_SERIAL.println(String("mqtt_server: ") + mqtt_server);
    DEBUG_SERIAL.println(String("mqtt_port: ") + mqtt_port);

    client.setServer(mqtt_server, atoi(mqtt_port));
    client.setCallback(callback);
    client.publish(topic_alive, "0");

    // Handle Web Server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html, processor);
    });

    // Handle Web Server Events
    events.onConnect([](AsyncEventSourceClient *client){
      if(client->lastId()){
        DEBUG_SERIAL.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      client->send("hello!", NULL, millis(), 10000);

    });

    
    // start AP Mode
    server.on("/apmode", HTTP_GET, [](AsyncWebServerRequest *request) {
      ESPAsync_WiFiManager wifiManager(&server,&dns);
      wifiManager.resetSettings();
      
      ESP.restart();
      request->send_P(200, "text/html", index_html, processor);
    });


    // restart system
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {   
      DEBUG_SERIAL.println("/restart");  
      ESP.restart();
      request->send_P(200, "text/html", index_html, processor);
    });


    server.on("/acoutput", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String parm1;
  
      // GET input1 value on <ESP_IP>/checkbox?output=<inputMessage1>&state=<inputMessage2>
      if (request->hasParam("value") ) {
        parm1 = request->getParam("value")->value();
        DEBUG_SERIAL.print("/acoutput?value = ");
        DEBUG_SERIAL.println(parm1);

        if(parm1.equals("/s0") ){
          value_power = 0;
          sprintf(msgData, "%d",value_power);
          client.publish(topic_power, msgData);
          newData = true;
        }
        else if(parm1.equals("/p1")){
          value_power +=1;
          sprintf(msgData, "%d",value_power);
          client.publish(topic_power, msgData);
          newData = true;
        }
        else if(parm1.equals("/p10")){
          value_power +=10;
          sprintf(msgData, "%d",value_power);
          client.publish(topic_power, msgData);
          newData = true;
        }
        else if(parm1.equals("/m1")){
          value_power -=1;
          if(value_power < 0){
            value_power = 0;
           }
          sprintf(msgData, "%d",value_power);
          client.publish(topic_power, msgData);
          newData = true;
        }
        else if(parm1.equals("/m10")){
          value_power -=10;
          if(value_power < 0){
            value_power = 0;
           }
          sprintf(msgData, "%d",value_power);
          client.publish(topic_power, msgData);
          newData = true;
        }
        
      }
     
      request->send_P(200, "text/html", index_html, processor);
    });


    
    server.on("/checkbox", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String checkbox_id;
      String checkbox_value;
     
      if (request->hasParam("cbid") && request->hasParam("state")) {
        checkbox_id = request->getParam("cbid")->value();
        checkbox_value = request->getParam("state")->value();
        DEBUG_SERIAL.print("Checkbox id=: ");
        DEBUG_SERIAL.println( checkbox_id);
        DEBUG_SERIAL.print("Checkbox value: ");
        DEBUG_SERIAL.println(checkbox_value);

        if(checkbox_id.equals("cb1")){
          DEBUG_SERIAL.println("1 checkbox_id = cb1");
          if(checkbox_value.equals("1")){
            DEBUG_SERIAL.println("2 checkbox_value = 1");
            checkbox1 = "<input type='checkbox' onchange='toggleCheckbox(this)' id='cb1' checked/>";
            checkboxT1 = true;
          }
          else {
            DEBUG_SERIAL.println("3 checkbox_value = 0");
            checkbox1 = "<input type='checkbox' onchange='toggleCheckbox(this)' id='cb1'/>";
            checkboxT1 = false;
          }
        }
        else if(checkbox_id.equals("cb2")){
          DEBUG_SERIAL.println("1 checkbox_id = cb2");
          if(checkbox_value.equals("1")){
            DEBUG_SERIAL.println("2 checkbox_value = 1");
            checkbox2 = "<input type='checkbox' onchange='toggleCheckbox(this)' id='cb2' checked/>";
            checkboxT2 = true;
          }
          else {
            DEBUG_SERIAL.println("3 checkbox_value = 0");
            checkbox2 = "<input type='checkbox' onchange='toggleCheckbox(this)' id='cb2'/>";
            checkboxT2 = false;
          }
        }
      }
      
      request->send_P(200, "text/html", index_html, processor);
    });


    server.on("/savetime", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String value1;
      String value2;
      // GET input1 value on <ESP_IP>/checkbox?output=<inputMessage1>&state=<inputMessage2>
      if (request->hasParam("t1") && request->hasParam("w1")) {
        
        value1 = request->getParam("t1")->value();
        value2 = request->getParam("w1")->value();

        memset(time_1, 0, sizeof(time_1)); 
        strcat(time_1, value1.c_str());     
        
        memset(watt_1, 0, sizeof(watt_1)); 
        strcat(watt_1, value2.c_str());     
        saveConfig();

        time1 = "<input type='time' id='t1' value='" + String(time_1) + "'/>"; 
        watt1 = "<input type='number' min='0' max='2000' value='" + String(watt_1) + "' id='w1'/>";


        DEBUG_SERIAL.print("time1=: ");
        DEBUG_SERIAL.println(value1);
        DEBUG_SERIAL.print("watt1: ");
        DEBUG_SERIAL.println(value2);
      }
      else if (request->hasParam("t2") && request->hasParam("w2")) {
        
        value1 = request->getParam("t2")->value();
        value2 = request->getParam("w2")->value();

        memset(time_2, 0, sizeof(time_2)); 
        strcat(time_2, value1.c_str());     
        
        memset(watt_2, 0, sizeof(watt_2)); 
        strcat(watt_2, value2.c_str());     
        saveConfig();  
       
        time2 = "<input type='time' id='t2' value='" + String(time_2) + "'/>";
        watt2 = "<input type='number' min='0' max='2000' value='" + String(watt_2) + "' id='w2'/>";

        DEBUG_SERIAL.print("time2=: ");
        DEBUG_SERIAL.println(value1);
        DEBUG_SERIAL.print("watt2: ");
        DEBUG_SERIAL.println(value2);
      }
         
      request->send_P(200, "text/html", index_html, processor);
    });

   
    AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA

    server.onNotFound(notFound);
    server.addHandler(&events);
    server.begin();
    DEBUG_SERIAL.println("Server start");

    //initial update webif & uptime  & mqtt_root 
    rssi = WiFi.RSSI();
    events.send(String(rssi).c_str(), "wifirssi", millis());
    DEBUG_SERIAL.print("send wifirssi: ");
    DEBUG_SERIAL.println(String(rssi).c_str());

    myUptime();
    events.send(Uptime_Str, "uptime", millis());
    DEBUG_SERIAL.print("send uptime: ");
    DEBUG_SERIAL.println(Uptime_Str);

    events.send(mqtt_root, "mqttroot", millis());
    DEBUG_SERIAL.print("send mqttroot: ");
    DEBUG_SERIAL.println(mqtt_root);


    events.send(watt_1, "w1", millis());
    DEBUG_SERIAL.print("send watt1: ");
    DEBUG_SERIAL.println(watt_1);

    events.send(watt_2, "w2", millis());
    DEBUG_SERIAL.print("send watt2: ");
    DEBUG_SERIAL.println(watt_2);

    digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_TX_PIN_VALUE); // Init transmit
    
  }

  // end setup()  
}



void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
    
  //send power to SoyoSource every 1000ms
  if ((millis() - lastTime1) > timerDelay1) {
    //digitalWrite(SERIAL_COMMUNICATION_CONTROL_PIN, RS485_TX_PIN_VALUE); // Init transmit

    if(newData == true && value_power >=0){
      setSoyoPowerData(value_power);
      events.send(String(value_power).c_str(),"staticpower", millis()); // nur an Website senden wenn es ein update vom Power_value gibt! neu 23.04.2023
      newData = false;
    }
    
    //daten an RS485 senden
    for(int i=0; i<8; i++){
      RS485Serial.write(soyo_power_data[i]);
      DEBUG_SERIAL.print(soyo_power_data[i], HEX);
      DEBUG_SERIAL.print(" ");
    }
   
    client.publish(topic_alive, "1");

    DEBUG_SERIAL.println();

    
    
    if(checkboxT1 || checkboxT2){
      time(&now);                       // read the current time
      localtime_r(&now, &tm);           // update the structure tm with the current time
    
      DEBUG_SERIAL.print("\thour:");
      DEBUG_SERIAL.print(tm.tm_hour);         // hours since midnight  0-23
      DEBUG_SERIAL.print("\tmin:");
      DEBUG_SERIAL.print(tm.tm_min);          // minutes after the hour  0-59
      DEBUG_SERIAL.print("\tsec:");
      DEBUG_SERIAL.print(tm.tm_sec);          // minutes after the hour  0-59
      DEBUG_SERIAL.println();
    }
  
    
    if (checkboxT1 == true){      
      int t1_hour = String(time_1).substring(0,2).toInt();
      int t1_min = String(time_1).substring(3).toInt();

      if((tm.tm_hour == t1_hour && tm.tm_min == t1_min && tm.tm_sec == 0) || (tm.tm_hour == t1_hour && tm.tm_min == t1_min && tm.tm_sec == 1) ){
        value_power = atoi(watt_1);
        sprintf(msgData, "%d", value_power);
        client.publish(topic_power, msgData);
        newData = true;

        DEBUG_SERIAL.print("Die Uhrzeit stimmt überein!");
        DEBUG_SERIAL.println();

        DEBUG_SERIAL.print("Leistung nach Uhrzeit: ");
        DEBUG_SERIAL.print(value_power);
        DEBUG_SERIAL.println();
      }
    }


    if (checkboxT2 == true){    
      int t2_hour = String(time_2).substring(0,2).toInt();
      int t2_min = String(time_2).substring(3).toInt();  
      
      if((tm.tm_hour == t2_hour && tm.tm_min == t2_min && tm.tm_sec == 0) || (tm.tm_hour == t2_hour && tm.tm_min == t2_min && tm.tm_sec == 1)){
        value_power = atoi(watt_2);
        sprintf(msgData, "%d", value_power);
        client.publish(topic_power, msgData);
        newData = true;

        DEBUG_SERIAL.print("Die Uhrzeit stimmt überein!");
        DEBUG_SERIAL.println();

        DEBUG_SERIAL.print("Leistung nach Uhrzeit: ");
        DEBUG_SERIAL.print(value_power);
        DEBUG_SERIAL.println();
      }
    }
    //******************************************************  
    
    

    lastTime1 = millis();


  }
   
}


