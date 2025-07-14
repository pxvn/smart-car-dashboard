#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <MPU6050.h>
#include "web.h"

// WiFi credentials
const char* ssid = "0000";
const char* password = "12121212";

// Pin definitions
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define BUZZER_PIN 5
#define NEOPIXEL_PIN 2
#define NEOPIXEL_COUNT 1
#define LEFT_LED 18
#define RIGHT_LED 19
#define LEFT_BUTTON 32
#define RIGHT_BUTTON 33
#define FRONT_TRIG 26
#define FRONT_ECHO 27
#define BACK_TRIG 14
#define BACK_ECHO 12
#define SDA_PIN 21
#define SCL_PIN 22

// Initialize components
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
AsyncWebServer server(80);
MPU6050 mpu;

// Global variables
volatile bool leftIndicator = false;
volatile bool rightIndicator = false;
bool obstacleAlert = false;
String obstacleLocation = "Clear";
float temperature = 20.0;
float humidity = 50.0;
int frontDistance = 0;
int backDistance = 0;
float speed = 0;
float direction = 0;
bool wifiConnected = false;
float accelBaseline = 16384.0;
bool mpuConnected = false;

// Timing variables
unsigned long lastIndicatorToggle = 0;
unsigned long lastSensorRead = 0;
unsigned long lastDHTRead = 0;
unsigned long lastMPURead = 0;
unsigned long lastWiFiAnimation = 0;
unsigned long lastButtonCheck = 0;

// Button states
bool lastLeftState = HIGH;
bool lastRightState = HIGH;
unsigned long leftDebounceTime = 0;
unsigned long rightDebounceTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Initialize pins first
  pinMode(LEFT_LED, OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(FRONT_TRIG, OUTPUT);
  pinMode(FRONT_ECHO, INPUT);
  pinMode(BACK_TRIG, OUTPUT);
  pinMode(BACK_ECHO, INPUT);
  
  // Initialize components
  dht.begin();
  strip.begin();
  strip.clear();
  strip.show();
  
  // Initialize I2C and MPU6050
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);
  
  mpu.initialize();
  mpuConnected = mpu.testConnection();
  if (mpuConnected) {
    Serial.println("MPU6050 connected");
    calibrateSpeed();
  } else {
    Serial.println("MPU6050 not found");
  }
  
  // WiFi connection with animation
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    handleWiFiAnimation();
    Serial.print(".");
    delay(500);
  }
  
  wifiConnected = true;
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
  
  // Show connection success
  for (int i = 0; i < 3; i++) {
    strip.setPixelColor(0, strip.Color(0, 255, 0));
    strip.show();
    delay(200);
    strip.clear();
    strip.show();
    delay(200);
  }
  
  setupWebServer();
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Handle button inputs (every 20ms)
  if (currentTime - lastButtonCheck >= 20) {
    handleButtons();
    lastButtonCheck = currentTime;
  }
  
  // Read ultrasonic sensors (every 100ms)
  if (currentTime - lastSensorRead >= 100) {
    readUltrasonicSensors();
    lastSensorRead = currentTime;
  }
  
  // Read DHT sensor (every 2 seconds)
  if (currentTime - lastDHTRead >= 2000) {
    readDHTSensor();
    lastDHTRead = currentTime;
  }
  
  // Read MPU6050 (every 100ms)
  if (currentTime - lastMPURead >= 100) {
    readMPU6050();
    lastMPURead = currentTime;
  }
  
  // Handle indicators (every 500ms)
  if (currentTime - lastIndicatorToggle >= 500) {
    handleIndicatorBlinking();
    lastIndicatorToggle = currentTime;
  }
  
  handleObstacleAlert();
  handleAmbientLighting();
  
  delay(10);
}

void handleWiFiAnimation() {
  static int brightness = 0;
  static int direction = 10;
  
  if (millis() - lastWiFiAnimation > 50) {
    brightness += direction;
    if (brightness >= 255) {
      brightness = 255;
      direction = -10;
    } else if (brightness <= 0) {
      brightness = 0;
      direction = 10;
    }
    
    strip.setPixelColor(0, strip.Color(0, 0, brightness));
    strip.show();
    lastWiFiAnimation = millis();
  }
}

void calibrateSpeed() {
  if (!mpuConnected) return;
  
  delay(1000);
  long totalAccel = 0;
  int validReadings = 0;
  
  for (int i = 0; i < 50; i++) {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    
    if (ax != 0 || ay != 0 || az != 0) {
      totalAccel += sqrt(ax*ax + ay*ay + az*az);
      validReadings++;
    }
    delay(20);
  }
  
  if (validReadings > 0) {
    accelBaseline = totalAccel / validReadings;
    Serial.println("Speed calibrated. Baseline: " + String(accelBaseline));
  }
}

void handleButtons() {
  bool leftReading = digitalRead(LEFT_BUTTON);
  bool rightReading = digitalRead(RIGHT_BUTTON);
  
  // Left button with debouncing
  if (leftReading != lastLeftState) {
    leftDebounceTime = millis();
  }
  
  if ((millis() - leftDebounceTime) > 50) {
    if (leftReading == LOW && lastLeftState == HIGH) {
      leftIndicator = !leftIndicator;
      if (leftIndicator) rightIndicator = false;
      Serial.println("Left indicator: " + String(leftIndicator));
    }
  }
  lastLeftState = leftReading;
  
  // Right button with debouncing
  if (rightReading != lastRightState) {
    rightDebounceTime = millis();
  }
  
  if ((millis() - rightDebounceTime) > 50) {
    if (rightReading == LOW && lastRightState == HIGH) {
      rightIndicator = !rightIndicator;
      if (rightIndicator) leftIndicator = false;
      Serial.println("Right indicator: " + String(rightIndicator));
    }
  }
  lastRightState = rightReading;
}

void readUltrasonicSensors() {
  // Read front sensor
  digitalWrite(FRONT_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(FRONT_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(FRONT_TRIG, LOW);
  
  long frontDuration = pulseIn(FRONT_ECHO, HIGH, 30000);
  frontDistance = (frontDuration == 0) ? 0 : (frontDuration * 0.034 / 2);
  
  // Read back sensor
  digitalWrite(BACK_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(BACK_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(BACK_TRIG, LOW);
  
  long backDuration = pulseIn(BACK_ECHO, HIGH, 30000);
  backDistance = (backDuration == 0) ? 0 : (backDuration * 0.034 / 2);
  
  // Update obstacle detection
  bool frontObstacle = (frontDistance > 0 && frontDistance < 30);
  bool backObstacle = (backDistance > 0 && backDistance < 30);
  
  obstacleAlert = frontObstacle || backObstacle;
  
  if (frontObstacle && backObstacle) {
    obstacleLocation = "Both";
  } else if (frontObstacle) {
    obstacleLocation = "Front";
  } else if (backObstacle) {
    obstacleLocation = "Back";
  } else {
    obstacleLocation = "Clear";
  }
}

void readDHTSensor() {
  float newHumidity = dht.readHumidity();
  float newTemperature = dht.readTemperature();
  
  if (!isnan(newHumidity) && !isnan(newTemperature)) {
    humidity = newHumidity;
    temperature = newTemperature;
    Serial.println("Temp: " + String(temperature) + "°C, Humidity: " + String(humidity) + "%");
  }
}

void readMPU6050() {
  if (!mpuConnected) return;
  
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // Calculate speed based on acceleration change
  float accelMagnitude = sqrt(ax*ax + ay*ay + az*az);
  float accelDiff = abs(accelMagnitude - accelBaseline);
  
  if (accelDiff > 1000) {
    speed = map(accelDiff, 1000, 10000, 0, 50);
    speed = constrain(speed, 0, 50);
  } else {
    speed = 0;
  }
  
  // Update direction based on gyroscope
  if (abs(gz) > 500) {
    direction += (gz / 131.0) * 0.1;
    if (direction >= 360) direction -= 360;
    if (direction < 0) direction += 360;
  }
}

void handleIndicatorBlinking() {
  static bool blinkState = false;
  blinkState = !blinkState;
  
  if (leftIndicator) {
    digitalWrite(LEFT_LED, blinkState ? HIGH : LOW);
    if (blinkState) {
      tone(BUZZER_PIN, 1000, 100);
    }
  } else {
    digitalWrite(LEFT_LED, LOW);
  }
  
  if (rightIndicator) {
    digitalWrite(RIGHT_LED, blinkState ? HIGH : LOW);
    if (blinkState) {
      tone(BUZZER_PIN, 1000, 100);
    }
  } else {
    digitalWrite(RIGHT_LED, LOW);
  }
}

void handleObstacleAlert() {
  if (obstacleAlert) {
    // Flash red for obstacle
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    strip.show();
    
    // Distance-based beeping
    int minDistance = 999;
    if (frontDistance > 0) minDistance = min(minDistance, frontDistance);
    if (backDistance > 0) minDistance = min(minDistance, backDistance);
    
    if (minDistance < 999) {
      int beepDelay = map(minDistance, 1, 30, 100, 1000);
      static unsigned long lastBeep = 0;
      
      if (millis() - lastBeep > beepDelay) {
        int frequency = map(minDistance, 1, 30, 2000, 800);
        tone(BUZZER_PIN, frequency, 50);
        lastBeep = millis();
      }
    }
  }
}

void handleAmbientLighting() {
  if (!obstacleAlert) {
    if (temperature < 20) {
      strip.setPixelColor(0, strip.Color(0, 100, 255)); // Blue - cold
    } else if (temperature < 25) {
      strip.setPixelColor(0, strip.Color(0, 255, 100)); // Green - comfortable
    } else {
      strip.setPixelColor(0, strip.Color(255, 150, 0)); // Orange - warm
    }
    strip.show();
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", getWebPage());
  });
  
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    DynamicJsonDocument doc(1024);
    
    doc["temperature"] = round(temperature * 10) / 10.0;
    doc["humidity"] = round(humidity * 10) / 10.0;
    doc["leftIndicator"] = leftIndicator;
    doc["rightIndicator"] = rightIndicator;
    doc["obstacleAlert"] = obstacleAlert;
    doc["obstacleLocation"] = obstacleLocation;
    doc["frontDistance"] = frontDistance;
    doc["backDistance"] = backDistance;
    doc["speed"] = round(speed * 10) / 10.0;
    doc["direction"] = round(direction * 10) / 10.0;
    
    // Calculate minimum distance for display
    int minDistance = 0;
    if (obstacleAlert) {
      minDistance = 999;
      if (frontDistance > 0) minDistance = min(minDistance, frontDistance);
      if (backDistance > 0) minDistance = min(minDistance, backDistance);
      if (minDistance == 999) minDistance = 0;
    }
    doc["minDistance"] = minDistance;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/api/indicator/left", HTTP_POST, [](AsyncWebServerRequest *request){
    leftIndicator = !leftIndicator;
    if (leftIndicator) rightIndicator = false;
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  server.on("/api/indicator/right", HTTP_POST, [](AsyncWebServerRequest *request){
    rightIndicator = !rightIndicator;
    if (rightIndicator) leftIndicator = false;
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
}
