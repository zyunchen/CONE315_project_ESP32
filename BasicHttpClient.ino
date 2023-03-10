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

#define USE_SERIAL Serial

const char* ssid = "OpenWrt24";
const char* password = "17708499678cyk!";

const char* serverName = "http://192.168.1.249:8086/smartnode/humidity";
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

  int httpResponseCode = http.POST(("{\"humdity\":\"" + std::to_string(humidity) + "\"}").c_str());

  String playload = "{}"; 
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    playload = http.getString();
    Serial.println(playload);
    
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
  digitalWrite(PUMP, HIGH);
  digitalWrite(LED, HIGH);
  sendHumidty(getHumidity());
  
  
}

void stopWatering(){
  // set pump and led off
  digitalWrite(PUMP, LOW);
  digitalWrite(LED, LOW); 
  sendHumidty(getHumidity()); 
}

void ifShouldWater(int curHumidity){
  if (curHumidity < wateringThreshold){
    beginWatering();
    while(1){
      delay(500);
      curHumidity = getHumidity();
      if (curHumidity > (wateringThreshold + 20) or curHumidity > 90){   
        stopWatering();   
      }   
    }
  }
    
  

}


void loop() {
  //Send an HTTP POST request every 10 minutes
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
