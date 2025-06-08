//This is the main PID code for the Cam87 Astrogatines Edition.

#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
#else
  // Arduino.h, ESP8266WiFi.h, etc. commented out as per original
#endif
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AsyncTCP.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <PID_v1.h>

// PID Configuration
double Setpoint, Input, Outp;
double Kp = 200, Ki = 10, Kd = 5;
PID myPID(&Input, &Outp, &Setpoint, Kp, Ki, Kd, DIRECT);
int WindowSize = 5000;
unsigned long windowStartTime;

// PID Adaptive Configuration
float ambientTemp = 0.0; // Stores initial ambient temperature
bool ambientRecorded = false;
const float maxCoolingDelta = 20.0; // Maximum cooling capability (20°C below ambient)

// Event and JSON variables
AsyncEventSource events("/events");
JSONVar readings;

// Sensor pins and instances
#define ONE_WIRE_BUS 4
#define DHTPIN 0
#define DHTTYPE DHT22    

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHTTYPE);

// Temperature variables
char temperatureC[10] = "";
float dht_h;
float dht_t;
float tempC;
char dewPointStr[10] = "0.0"; // Stores last calculated Dew Point

// Timer variables
unsigned long lastTime = 0;  
unsigned long lastDewPCalc = 0;
unsigned long timerDelay = 10000;
const unsigned long dewPCalcInterval = 30000; // 30 seconds for DewP
const int output = 2;

// Threshold and control variables
char inputMessage[10] = "25.0";
char enableArmChecked[10] = "checked";
char inputMessage2[10] = "false";
char coolingstate[10] = "OFF";

// Network credentials
const char* ssid = "Cam87 Cooling";
const char* password = "299792458";

// Parameter names
const char* PARAM_INPUT_1 = "threshold_input";
const char* PARAM_INPUT_2 = "enable_arm_input";

// Web server
AsyncWebServer server(80);

// Helper function to convert float to char array
void floatToChar(float value, char* output, int precision = 1) {
  dtostrf(value, 0, precision, output);
}

void updatePIDParameters() {
  if (ambientRecorded) {
    float delta = ambientTemp - Setpoint;
    
    // Constrain delta to system capabilities
    delta = constrain(delta, 0, maxCoolingDelta);
    
    // Adaptive PID tuning rules
    if (delta > 15.0) {
      // Large delta - aggressive tuning
      Kp = 300;
      Ki = 15;
      Kd = 10;
    } 
    else if (delta > 5.0) {
      // Medium delta - moderate tuning
      Kp = 200;
      Ki = 10;
      Kd = 5;
    }
    else {
      // Small delta - conservative tuning
      Kp = 100;
      Ki = 5;
      Kd = 2;
    }
    
    // Apply new parameters
    myPID.SetTunings(Kp, Ki, Kd);
    
    Serial.print("Delta: ");
    Serial.print(delta);
    Serial.print("°C - New PID values: Kp=");
    Serial.print(Kp);
    Serial.print(", Ki=");
    Serial.print(Ki);
    Serial.print(", Kd=");
    Serial.println(Kd);
  }
}

// Sensor reading functions
void getSensorReadings(char* jsonOutput, size_t size) {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0); // Reads the first sensor found
  char tempStr[10];
  floatToChar(temp, tempStr);
  
  snprintf(jsonOutput, size, "{\"temperature\":\"%s\"}", tempStr); 
}

void initLittleFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else {
    Serial.println("LittleFS mounted successfully");
  }
}

void readDHTTemperature(char* output, size_t size) {
  float readt = dht.readTemperature();
  if (isnan(readt)) {    
    Serial.println("Failed to read from DHT sensor!");
    strncpy(output, "0.0", size);
    dht_t = 0.0;
  }
  else {
    if (readt > 0) { dht_t = readt; }
    floatToChar(dht_t, output);
  }
}

void readDHTHumidity(char* output, size_t size) {
  float readh = dht.readHumidity();
  if (isnan(readh)) {
    Serial.println("Failed to read from DHT sensor!");
    strncpy(output, "0.0", size);
    dht_h = 0.1;
  }
  else {
    dht_h = readh;
    floatToChar(dht_h, output);
  }
}

void calculateDewPoint() {
  if (millis() - lastDewPCalc >= dewPCalcInterval) {
    // Use 0.1% when MEASURED humidity is 0.0%
    float calc_h = (fabs(dht_h) < 0.0001f) ? 0.1f : dht_h;
    
    // Magnus formula for dew point
    float DewP = (237.7f*(logf(calc_h/100.0f)+(17.27f*dht_t)/(237.7f+dht_t)))/
                 (17.27f-(logf((calc_h/100.0f))+(17.27f*dht_t)/(237.7f+dht_t)));
    
    if (isnan(DewP)) {
      strncpy(dewPointStr, "0.0", sizeof(dewPointStr));
      Serial.println("DewP calculation error!");
    } else {
      floatToChar(DewP, dewPointStr, 1);
      Serial.print("Calculated DewP: ");
      Serial.println(DewP);
    }
    lastDewPCalc = millis();
  }
}

void readDSTemperatureC(char* output, size_t size) {
  sensors.requestTemperatures(); 
  tempC = sensors.getTempCByIndex(0);

  if(tempC == -127.00) {
    Serial.println("Failed to read from DS18B20 sensor");
    strncpy(output, "--", size);
  } 
  else {
    floatToChar(tempC, output);
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
</html>)rawliteral";

void processor(const String& var, char* output, size_t size){
  if(var == "TEMPERATUREC"){
    strncpy(output, temperatureC, size);
  }
  else if(var == "TEMPERATURE"){
    char temp[10];
    readDHTTemperature(temp, sizeof(temp));
    strncpy(output, temp, size);
  }
  else if(var == "HUMIDITY"){
    char hum[10];
    readDHTHumidity(hum, sizeof(hum));
    strncpy(output, hum, size);
  }
  else if(var == "DewP"){
    strncpy(output, dewPointStr, size);
  }
  else if(var == "inputMessage"){
    strncpy(output, inputMessage, size);
  }
  else if(var == "inputMessage2"){
    strncpy(output, coolingstate, size);
  }
  else {
    strncpy(output, "", size);
  }
}

void setup(){
  Serial.begin(115200);
  Serial.println();
  
  sensors.begin();
  dht.begin();
  
  // Record ambient temperature at startup
  float initialTemp = dht.readTemperature();
  if (!isnan(initialTemp)) {
    ambientTemp = initialTemp;
    ambientRecorded = true;
    Serial.print("Recorded ambient temperature: ");
    Serial.println(ambientTemp);
  }

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  initLittleFS();

  windowStartTime = millis();
  Setpoint = atof(inputMessage);
  updatePIDParameters();
  myPID.SetOutputLimits(0, WindowSize);
  myPID.SetMode(AUTOMATIC);

  readDSTemperatureC(temperatureC, sizeof(temperatureC));
  Serial.println();
  Serial.println(WiFi.localIP());

  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);

  // Initial Dew Point calculation
  calculateDewPoint();

  // Web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_INPUT_1)) {
      strncpy(inputMessage, request->getParam(PARAM_INPUT_1)->value().c_str(), sizeof(inputMessage));
      Setpoint = atof(inputMessage);
      updatePIDParameters();
      
      if (request->hasParam(PARAM_INPUT_2)) {
        strncpy(inputMessage2, request->getParam(PARAM_INPUT_2)->value().c_str(), sizeof(inputMessage2));
        strncpy(enableArmChecked, "checked", sizeof(enableArmChecked));
      }
      else {
        strncpy(inputMessage2, "false", sizeof(inputMessage2));
        strncpy(enableArmChecked, "", sizeof(enableArmChecked));
      }
    }

    if (strcmp(inputMessage2, "true") == 0) { 
      strncpy(coolingstate, "ON", sizeof(coolingstate));
    } 
    else { 
      strncpy(coolingstate, "OFF", sizeof(coolingstate));
    }
   
    Serial.println(inputMessage);
    Serial.println(inputMessage2);
    Serial.println(coolingstate);
    request->send(204, "text/html", "");
  });
  
  server.serveStatic("/", LittleFS, "/");
  
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    char json[100];
    getSensorReadings(json, sizeof(json));
    request->send(200, "application/json", json);
  });
  
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperatureC);
  });
  
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    char temp[10];
    readDHTTemperature(temp, sizeof(temp));
    request->send_P(200, "text/plain", temp);
  });
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    char hum[10];
    readDHTHumidity(hum, sizeof(hum));
    request->send_P(200, "text/plain", hum);
  });

  server.on("/DewP", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", dewPointStr);
  });

  server.on("/inputMessage", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", inputMessage);
  });  
  
  server.on("/coolingstate", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", coolingstate);
  });  

  server.begin();
}
 
void loop(){
  Input = tempC;
  
  // Update PID parameters if setpoint changes
  static float lastSetpoint = Setpoint;
  if (Setpoint != lastSetpoint) {
    updatePIDParameters();
    lastSetpoint = Setpoint;
  }
  
  myPID.Compute();
  
  if (millis() - windowStartTime > WindowSize) {
    windowStartTime += WindowSize;
  }
  
  if ((Outp < millis() - windowStartTime) && (strcmp(inputMessage2, "true") == 0)) {
    digitalWrite(output, HIGH);
  }
  else {
    digitalWrite(output, LOW);
  }

  // Calculate Dew Point every 30 seconds
  calculateDewPoint();

  if ((millis() - lastTime) > timerDelay) {
    readDSTemperatureC(temperatureC, sizeof(temperatureC));
    events.send("ping", NULL, millis());
    
    char json[100];
    getSensorReadings(json, sizeof(json));
    events.send(json, "new_readings", millis());
    
    lastTime = millis();
  }  
}