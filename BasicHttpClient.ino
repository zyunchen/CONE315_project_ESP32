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
#include <WiFiClient.h>
#include <WiFiAP.h>


// Set these to your desired credentials.
const char *open_ssid = "smart_plant_care";
const char *open_password = "Smartplant";

WiFiServer server(80);

#define USE_SERIAL Serial

// const char* ssid = "OpenWrt24";
// const char* password = "17708499678cyk";

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
  
  // start wifi server to let user set the wifi name and password that esp32 to connect to 
  WiFi.softAP(open_ssid,open_password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");
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

void setupWIFI(WiFiClient *client, String &requestLine){
  // [GET /setup?] = 11, [ HTTP/1.1] = 9
  requestLine = requestLine.substring(11, requestLine.length() - 9);

  String name = "";
  String pwd = "";
  while(1){
    int idx = requestLine.indexOf("&");
    String params;
    if(idx == -1){
      params = requestLine;
      requestLine = "";
    }else{
      params = requestLine.substring(0, idx);
      requestLine = requestLine.substring(idx + 1);      
    }
    
    idx = params.indexOf("=");
    if(idx == -1) break;
    String key = params.substring(0, idx);
    String value = params.substring(idx + 1);
    if(key == "name"){
      name = value;
    }else if(key == "password"){
      pwd = value;
    }
  }
  if(name == "") return;

  if(pwd == ""){
    WiFi.begin(name.c_str());
  }else{
    WiFi.begin(name.c_str(), pwd.c_str());
  }

  Serial.println("name: [" + name + "]");
  Serial.println("pwd: [" + pwd + "]");

  Serial.println("Connecting to an internet wifi...");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to an internet WiFi with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");

  while(client->available()){
    client->read();
  }

  client->println("HTTP/1.1 200 OK");
  client->println("Content-type:text/html");
  client->println();
  
  // The HTTP response ends with another blank line:
  client->println("Connected to an internet WiFi with IP Address: ");
  client->println(WiFi.localIP());
  client->println();
  client->flush();
  client->stop();
  delay(2000);
  WiFi.softAPdisconnect(true);
}

void serverLoop(){
  WiFiClient client = server.available();   // listen for incoming clients
  if (!client) return;
  
  Serial.println("New Client.");           // print a message out the serial port
  String currentLine = "";                // make a String to hold incoming data from the client
  while (client.connected()) {            // loop while the client's connected
    if (!client.available()) continue;
    
    char c = client.read();             // read a byte, then
    Serial.write(c);                    // print it out the serial monitor
    if (c == '\n') {                    // if the byte is a newline character
      // if the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if(currentLine.startsWith("GET /setup?")){
        setupWIFI(&client, currentLine);
        return;
      }else if (currentLine.length() == 0) {
        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
        // and a content-type so the client knows what's coming, then a blank line:
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        
        // The HTTP response ends with another blank line:
        client.println("<html><style>input[type=\"text\"] {    font-size:3em;}input[type=\"submit\"] {    font-size:3em;}label{ font-size:3em;}</style><body align =\"center\"><h1>Setup an internet WIFI</h1><form action=\"/setup\" method=\"GET\">  <label for=\"name\">WIFI name:</label><br>  <input type=\"text\" id=\"name\" name=\"name\" height=\"100\"><br>  <label for=\"password\">WIFI Password:</label><br>  <input type=\"text\" id=\"password\" name=\"password\"><br><br>  <input type=\"submit\" value=\"Submit\"></form> </body></html>");
        // break out of the while loop:
        break;
      } else {    // if you got a newline, then clear currentLine:
        currentLine = "";
      }
    } else if (c != '\r') {  // if you got anything else but a carriage return character,
      currentLine += c;      // add it to the end of the currentLine
    }
    // Check to see if the client request was "GET /H" or "GET /L":
    if (currentLine.endsWith("GET /on1")) {
      digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
    }
    if (currentLine.endsWith("GET /off1")) {
      digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
    }
  }
  // close the connection:
  client.stop();
  Serial.println("Client Disconnected.");
}

void loop() {
  serverLoop();
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
