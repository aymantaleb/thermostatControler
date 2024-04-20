#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
// #include "config.h" 



void IRAM_ATTR handleTempUpPress();
void IRAM_ATTR handleTempDownPress();
void IRAM_ATTR handleModeSetPress();
void cycleModes();
void thermoStatInfoGET();
void connectToWifi();
void reconnectWifiIfNeeded();
String httpGETRequest(const char* serverName);
String httpPOSTRequest(const char* serverName, String params);


// const char* ssid = "YOUR_WIFI_SSID";
// const char* password = "YOUR_WIFI_PASSWORD";
// const char* thermostatIp = "YOUR_THERMOSTAT_IP";
WebServer server(80);

// lcd init
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Global variable to track the last update time
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 120000;  // 2 minutes in milliseconds

//thermostat info
String thermostatMode = "";
String thermostatState = "";
int tempToSet  = 0;
int thermostatInfoCodes[5] = {0,0,0,0,0};
int thermostatModes[3] = {0,1,2}; //"OFF", "COOL", "HEAT", "AUTO"
int modeToSet = 0;
int currentModeIndex = 0;



// button and led pin init
const int tempUpPin = 15;
const int tempDownPin = 19;
const int modeSetPin = 4;
const int redLed = 12;
const int blueLed = 32;

volatile bool tempUpPressed = false;
volatile bool tempDownPressed = false;
volatile bool modeSetPressed = false;
volatile bool changeMade = false; //tracks whether or not user made a change, temp or mode, and updates the screen accordingly


//button debounce functions
void IRAM_ATTR handleTempUpPress() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 30) {
    tempUpPressed = true;
    changeMade = true;
    lastInterruptTime = interruptTime;
  }
}

void IRAM_ATTR handleTempDownPress() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 30) {
    tempDownPressed = true;
    changeMade = true;
    lastInterruptTime = interruptTime;
  }
}

void IRAM_ATTR handleModeSetPress() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 30) {
    modeSetPressed = true;
    changeMade = true;
    lastInterruptTime = interruptTime;
  }
}

//cycle through different thermostat modes 
void cycleModes() {
  currentModeIndex++;
  if (currentModeIndex >= 3) { 
    currentModeIndex = 0;
  }

  modeToSet = thermostatModes[currentModeIndex];
  Serial.print("Mode set to: ");
  Serial.println(modeToSet); 
}


void setup()
{

  Serial.begin(115200);

  // lcd init block
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("READY");
  delay(250);
  lcd.clear();

  // wifi connection
  connectToWifi();

  // i/o init
  pinMode(tempUpPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(tempUpPin), handleTempUpPress, FALLING);
  pinMode(tempDownPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(tempDownPin), handleTempDownPress, FALLING);
  pinMode(modeSetPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(modeSetPin), handleModeSetPress, FALLING);
  pinMode(redLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  digitalWrite(redLed, LOW);
  digitalWrite(blueLed, LOW);
  thermoStatInfoGET();
  tempToSet = thermostatInfoCodes[4];

}

void loop()
{
  lcd.clear();

   if (modeSetPressed) {
      
      Serial.println("modeSet Pressed");
      cycleModes();
      modeSetPressed = false;
    } 

    if(tempUpPressed){
      Serial.println("tempUp Pressed");
      tempToSet++;
      Serial.println(tempToSet);
      tempUpPressed = false;            
    }

    if(tempDownPressed){
      Serial.println("tempDown Pressed");
      tempToSet--;
      Serial.println(tempToSet);
      tempDownPressed = false;
    }

  if (WiFi.status() == WL_CONNECTED)
  {
   

    unsigned long currentMillis = millis();
    if(changeMade){
      delay(200);
      String postCall = thermostatIp + "/control?";
      String postParams = "mode=" + String(modeToSet) + "&heattemp=" + String(tempToSet) + "&cooltemp=" + String(tempToSet); 
      Serial.println(postCall + postParams);
      String thermostatResponse = httpPOSTRequest(postCall.c_str(),postParams); 
      Serial.println(thermostatResponse);
      thermoStatInfoGET();
      changeMade = false;
    }
    else if ((currentMillis - lastUpdateTime >= updateInterval)) {
      thermoStatInfoGET();
      Serial.println("INFO UPDATE");
      lastUpdateTime = currentMillis;
    }

    int setTemp = 0;
    if(thermostatInfoCodes[0] == 0){
      setTemp = 0;
      thermostatMode = "OFF";
      digitalWrite(redLed, LOW);
      digitalWrite(blueLed, LOW);
    }
    else if(thermostatInfoCodes[0] == 1){
      setTemp = thermostatInfoCodes[3];
      thermostatMode = "HEAT";
      digitalWrite(redLed, HIGH);
      digitalWrite(blueLed, LOW);
    }
    else if(thermostatInfoCodes[0] == 2){
      setTemp = thermostatInfoCodes[4];
      thermostatMode = "COOL";
      digitalWrite(redLed, LOW);
      digitalWrite(blueLed, HIGH);
    }
    else if(thermostatInfoCodes[0] == 3){
      setTemp = thermostatInfoCodes[4];
      thermostatMode = "AUTO";
      digitalWrite(redLed, HIGH);
      digitalWrite(blueLed, HIGH);
    }


    if(thermostatInfoCodes[1] == 0){
      thermostatState = "IDLE";
    }
    else if(thermostatInfoCodes[1] == 1){
      thermostatState = "HEAT";
    }
    else if(thermostatInfoCodes[1] == 2){
      thermostatState = "COOL";
    }
    else if(thermostatInfoCodes[1] == 3){
      thermostatState = "LOCK";
    }
    else if(thermostatInfoCodes[1] == 4){
      thermostatState = "ERROR";
    }

    String modeStr = "M: " + thermostatMode;
    String stateStr = "S: " + thermostatState;
    String ambTempStr = "AT: " + String(thermostatInfoCodes[2]);
    String setTempStr = "ST: " + String(setTemp);
    

    lcd.print(modeStr);
    lcd.print(" ");
    lcd.print(stateStr);
    lcd.setCursor(0, 1);
    lcd.print(ambTempStr);
    lcd.print(" ");
    lcd.print(setTempStr);
    
    delay(1000);
    
  }
    else
    {
      lcd.print("NO WIFI");
      // reconnectWifiIfNeeded();
      lcd.clear();
    }

    delay(500);
  }


//get current data from thermostat (ambient temp, set temp, mode, status)
void thermoStatInfoGET(){
  String infoCall = thermostatIp + "/query/info";
  String thermostatInfo = httpGETRequest(infoCall.c_str()); 
  if (thermostatInfo.length() == 0) {
      Serial.println("HTTP request failed or returned an empty response.");
      return;
   }

  Serial.println(thermostatInfo);
  JSONVar myObject = JSON.parse(thermostatInfo);
  if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }
  JSONVar keys = myObject.keys();
  int tempArr[5] = {myObject[keys[1]], myObject[keys[2]], myObject[keys[9]], myObject[keys[10]], myObject[keys[11]]}; //mode state ambTemp heatTemp coolTemp
  for(int i = 0; i < 5; i++){
    thermostatInfoCodes[i] = tempArr[i];
  } 
    
}

//connecting to WIFI
void connectToWifi()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  lcd.print("WiFi connected:)");
   
  delay(2000);
  lcd.clear();
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void reconnectWifiIfNeeded()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    connectToWifi();
  }
}

//generic HTTP GET function
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  String payload = "{}"; 

 
        // Your Domain name with URL path or IP address with path
        http.begin(client, serverName);

        // Send HTTP GET request
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            payload = http.getString();
        } else {
            Serial.print("****Error code: ");
            Serial.println(httpResponseCode);
            payload = "";
        }

        // Free resources
        http.end();
   

  return payload;
}

//generic HTTP POST function
String httpPOSTRequest(const char* serverName, String params) {
  WiFiClient client;
  HTTPClient http;
   String payload = "{}";

        // Your Domain name with URL path or IP address with path
        http.begin(client, serverName);

        // Send HTTP POST request
        int httpResponseCode = http.POST(params);
        if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            payload = http.getString();
        } else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
            payload = "";
        }

        // Free resources
        http.end();
    
  return payload;
}



