/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <SPI.h>
#include <Wire.h> 
#include <string> 
#include <esp_mac.h>
#include <time.h>

#define USE_SERIAL Serial

const char* ssid = "OpenWrt24";
const char* password = "17708499678cyk!";

const char* serverName = "http://110.40.205.204:8086/sensor/changeSensorData";
// String serverName = "http://192.168.1.106:1880/update-sensor";



#define LED 4   
#define PUMP 22




// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

String sensorReadings;
String sensorReadingsArr[3];

char macAddress[13];
int  status = 0; // 0 is no water, 1 is watering

 
const int AIR_VALUE = 3300;   //this value is when the humidity sensor put in the air
const int WATER_VALUE = 1700;  //this value is when the humidity sensor put in the water
const int SensorPin = 34;
int soilMoistureValue = 0;
int soilmoisturepercent=0;

int wateringThreshold = 15; // this is the threshold humidity, if the humidity below this, watering begin.





String httpGETRequest(const char* serverName) {
  // WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  // http.begin(client, serverName);
  http.begin(serverName);


  
  
  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void setup() {
  Serial.begin(115200); // open serial port, set the baud rate to 115200

  pinMode(LED, OUTPUT);
  pinMode(PUMP, OUTPUT);

  Serial.println("mac address is");
  
  uint8_t mac_address[6];
  esp_read_mac(mac_address,ESP_MAC_WIFI_STA);
  
  Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);

  sprintf(macAddress, "%02x%02x%02x%02x%02x%02x", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
  Serial.println(macAddress);
  

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}


void sendHumidty(int humidity){
  HTTPClient http;
  // Your Domain name with URL path or IP address with path
  http.begin(serverName);
  // Specify content-type header to json
  http.addHeader("Content-Type", "application/json");
  // Data to send with HTTP POST

  std::string mac = macAddress;

  Serial.println(("{\"sensorId\":\"" + mac + "\",\"currentHumidity\":\"" + std::to_string(humidity) + "\",\"status\":" + std::to_string(status) + "}").c_str());

  int httpResponseCode = http.POST(("{\"sensorId\":\"" + mac + "\",\"currentHumidity\":\"" + std::to_string(humidity) + "\",\"status\":" + std::to_string(status) + "}").c_str());

  String playload = "{}"; 
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    playload = http.getString();
    Serial.println(playload);
    
    JSONVar myObject = JSON.parse(playload);
    //Serial.println();
    wateringThreshold = myObject["data"]["threshold"];
    Serial.printf("wateringThreshold: %d\n",  wateringThreshold);
    // TODO: deal with the wateringThreshold
    
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();



      // sensorReadings = httpGETRequest(serverName);
      // Serial.println(sensorReadings);
      // JSONVar myObject = JSON.parse(sensorReadings);
  
      // // JSON.typeof(jsonVar) can be used to get the type of the var
      // if (JSON.typeof(myObject) == "undefined") {
      //   Serial.println("Parsing input failed!");
      //   return;
      // }
    
      // Serial.print("JSON object = ");
      // Serial.println(myObject);
    
      // // myObject.keys() can be used to get an array of all the keys in the object
      // JSONVar keys = myObject.keys();
    
      // for (int i = 0; i < keys.length(); i++) {
      //   JSONVar value = myObject[keys[i]];
      //   Serial.print(keys[i]);
      //   Serial.print(" = ");
      //   Serial.println(value);
      //   sensorReadingsArr[i] = (const char*)value;
      // }
      // Serial.print("1 = ");
      // Serial.println(sensorReadingsArr[0]);
      // Serial.print("2 = ");
      // Serial.println(sensorReadingsArr[1]);
      // Serial.print("3 = ");
      // Serial.println(sensorReadingsArr[2]);



  //   if (JSON.typeof(myObject) == "undefined") {
  //   Serial.println("Parsing input failed!");
  //   return;
  // }
  // Serial.print("JSON object = ");
  // Serial.println(myObject);
  // // myObject.keys() can be used to get an array of all the keys in the object
  // JSONVar keys = myObject.keys();
  // for (int i = 0; i < keys.length(); i++) {
  //   JSONVar value = myObject[keys[i]];
  //   Serial.print(keys[i]);
  //   Serial.print(" = ");
  //   Serial.println(value);
  //   sensorReadingsArr[i] = double(value);
  // }
  // Serial.print("1 = ");
  // Serial.println(sensorReadingsArr[0]);
  // Serial.print("2 = ");
  // Serial.println(sensorReadingsArr[1]);
  // Serial.print("3 = ");
  // Serial.println(sensorReadingsArr[2]);
  
}

int getHumidity(){
  soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, AIR_VALUE, WATER_VALUE, 0, 100);
  if(soilmoisturepercent > 100)
  {
    Serial.println("100 %");
    return 100;
  }
  else if(soilmoisturepercent <0)
  {
    Serial.println("0 %");
    return 0;
  }
  else if(soilmoisturepercent >=0 && soilmoisturepercent <= 100)
  {
    Serial.print(soilmoisturepercent);
    Serial.println("%");
    return soilmoisturepercent;
  }  

  
}

void beginWatering(){
  // set pump and  led on 
  Serial.println("func beginWateringr");
  digitalWrite(PUMP, HIGH);
  digitalWrite(LED, HIGH);
  status = 1;
  sendHumidty(getHumidity());
  
  
  
}

void stopWatering(){
  // set pump and led off
  Serial.println("func stopWatering");
  digitalWrite(PUMP, LOW);
  digitalWrite(LED, LOW); 
  status = 0;
  sendHumidty(getHumidity()); 
  
}

void ifShouldWater(int curHumidity){
  Serial.println("func isShouldWater");

  if (curHumidity < wateringThreshold){
    beginWatering();
    while(1){
      delay(500);
      curHumidity = getHumidity();
      if (curHumidity > (wateringThreshold + 20) or curHumidity > 90){   
        stopWatering();   
        break;
      }   
    }
  }
    
  

}


void loop() {
  //Send an HTTP POST request every 10 minutes
  ifShouldWater(getHumidity());
  delay(1000);
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      sendHumidty(getHumidity());
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

 
}
