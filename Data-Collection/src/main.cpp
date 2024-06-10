#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Bonezegei_ULN2003_Stepper.h>
#include <AHT10.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"
#include "DataSender.h"

#define DHT_TYPE DHT11
#define DHT11PIN 16
#define FORWARD 1
#define REVERSE 0

const char* serverUrl = "https://ece140.frosty-sky-f43d.workers.dev/api/insert";
const char* ucsdUsername = UCSD_USERNAME;
const char* ucsdPassword = UCSD_PASSWORD;
const char* ucsdPid = UCSD_PID;
const char* espLocation = ESP_LOCATION;
const char* wifiSsid = WIFI_SSID;
const char* nonEnterpriseWifiPassword = NON_ENTERPRISE_WIFI_PASSWORD;
const char* homewifi = "Sun";


// DataSender dataSender(ucsdPid, espLocation, serverUrl);
Bonezegei_ULN2003_Stepper Stepper(13, 12, 14, 27);
Adafruit_MPU6050 mpu;
AHT10 myAHT10(AHT10_ADDRESS_0X38);
LiquidCrystal_I2C lcd(0x27,20,4);
DataSender* data_sender = nullptr;

unsigned long previousMillis = 0;
const long interval = 2000;

// Button pins
const int buttonModePin = 15;
const int buttonIncrementPin = 4;
const int buttonEnPin = 5;

int setHour = 0;
int setMinute = 0;
bool isSet = false;
bool settingHour = false; // true if setting hour, false if setting minute

int lastButtonIncrementState = LOW;
int lastButtonEnState = LOW;

unsigned long lastLcdUpdateTime = 0; // For non-blocking LCD updates
const long lcdUpdateInterval = 60000; // Update LCD every second

unsigned long lastSensorUpdateTime = 0; // For non-blocking LCD updates
const long sensorUpdateInterval = 60000; // Update LCD every second

const float vibrationThreshold = 15; // Define your vibration threshold
bool lastVibrationState = false;

void setup() {
  Serial.begin(115200);
  lcd.init();                      // initialize the lcd 
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Connecting to wifi...");
  lcd.setCursor(0, 1);
  lcd.print("Set Time: None");
  // lcd.println("Processing...");
  // delay(5000); lcd.clear();

  // dataSender.connectToWiFi(wifiSsid, nonEnterpriseWifiPassword);
  data_sender = new DataSender(ucsdPid, "RIMAC", serverUrl);
  // data_sender->connectToWPAEnterprise(wifiSsid, ucsdUsername, ucsdPassword);
  data_sender->connectToWiFi(homewifi, "12345678");

  // while (!Serial)
  //   delay(10); // will pause until serial console opens

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  //Inititalize Pins
  Stepper.begin();

  // Speed in Milli seconds per step
  // Default Value is 3 
  Stepper.setSpeed(5);

    // Initialize button pins
  pinMode(buttonModePin, INPUT_PULLUP);
  pinMode(buttonIncrementPin, INPUT_PULLUP);
  pinMode(buttonEnPin, INPUT_PULLUP);

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Set Time: None");
}

void loop() {
  /* Get new sensor events with the readings */
  unsigned long currentMillis = millis();
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  // Button handling
  int buttonModeState = digitalRead(buttonModePin);
  int buttonIncrementState = digitalRead(buttonIncrementPin);
  int buttonEnState = digitalRead(buttonEnPin);

  if (buttonModeState == HIGH) {
    settingHour = true;
  } else {
    settingHour = false;
  }

  if((buttonEnState != lastButtonEnState) && (buttonEnState == HIGH)) {
    isSet = true;
  }
  lastButtonEnState = buttonEnState;

  if ((buttonEnState == LOW) && (buttonIncrementState != lastButtonIncrementState) && (buttonIncrementState == LOW)) {
    if (settingHour) {
      setHour = (setHour + 1) % 24;
    } else {
      setMinute = (setMinute + 5) % 60;
    }
    lcd.setCursor(10, 1);
    lcd.print(setHour < 10 ? "0" : ""); lcd.print(setHour); lcd.print(":");
    lcd.print(setMinute < 10 ? "0" : ""); lcd.print(setMinute); lcd.print(" ");
  }
  lastButtonIncrementState = buttonIncrementState;

  char setTime[5];
  // Display set time on LCD
  if (currentMillis - lastLcdUpdateTime >= lcdUpdateInterval) {
    data_sender->sendLocalTime();
    String localTime = data_sender->getTimeOnly();
    lastLcdUpdateTime = currentMillis; 
    lcd.setCursor(0, 0);
    lcd.print(localTime);
    sprintf(setTime, "%02d:%02d", setHour, setMinute);
    if (isSet && (strcmp(localTime.c_str(), setTime) == 0)){
      Stepper.step(FORWARD, 500);
      delay(3000);
      Stepper.step(REVERSE, 500);
      isSet = false;
    }
  }


  // if (currentMillis - previousMillis >= interval) {
  //   previousMillis = currentMillis;
  //   Stepper.step(FORWARD, 1000);
  // }

  // Stepper.step(REVERSE, 2000);
  // delay(2000);

 
  if (currentMillis - lastSensorUpdateTime >= sensorUpdateInterval) {
     /* Print out the values */
    Serial.print("Acceleration X: ");
    Serial.print(a.acceleration.x);
    Serial.print(", Y: ");
    Serial.print(a.acceleration.y);
    Serial.print(", Z: ");
    Serial.print(a.acceleration.z);
    Serial.println(" m/s^2");


    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degC");

    Serial.print(F("Humidity...: ")); Serial.print(myAHT10.readHumidity());    Serial.println(F(" +-2%"));   //by default "AHT10_FORCE_READ_DATA"

    Serial.println("");


    // Check for vibration
    bool isVibrating = abs(a.acceleration.x) > vibrationThreshold ||
                     abs(a.acceleration.y) > vibrationThreshold ||
                     abs(a.acceleration.z) > vibrationThreshold;


    // Send ON/OFF signal if the vibration state changes
    if (isVibrating != lastVibrationState) {
      lastVibrationState = isVibrating;
      String signal = isVibrating ? "ON" : "OFF";
      data_sender->sendData("Vibration", "Accelerometer", isVibrating ? 1 : 0, signal);
    }
  }

}




