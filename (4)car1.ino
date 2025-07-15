// =================================================================
//                 SMART CAR PROJECT - CAR 1 (MAIN)
// =================================================================

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <esp_wifi.h>
#include "web.h" // Include the provided web.h for the dashboard

// --- PIN DEFINITIONS ---
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

// --- WIFI CONFIG ---
const char *ssid = "SmartCar_Dashboard";
const char *password = "12345678";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Car 2 IP (will be assigned automatically)
String car2IP = "";
bool car2Connected = false;

// --- SENSOR OBJECTS ---
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_NeoPixel ambientLight(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_MPU6050 mpu;

// --- STATE VARIABLES ---
struct CarState {
    float temp = 0.0;
    float humidity = 0.0;
    float frontDist = 999.0;
    float backDist = 999.0;
    int speed = 0;
    float direction = 0;
    bool leftIndicator = false;
    bool rightIndicator = false;
    bool buzzerOn = true;
    bool ambientOn = true;
    bool car2Left = false;
    bool car2Right = false;
} carState;

// --- TIMING VARIABLES ---
unsigned long lastWsSend = 0;
unsigned long lastSensorRead = 0;
unsigned long lastIndicatorBlink = 0;
unsigned long lastCar2Check = 0;
unsigned long lastCar2Send = 0;
bool indicatorState = false;
float compassDirection = 0;

// --- BUTTON VARIABLES ---
volatile bool leftButtonPressed = false;
volatile bool rightButtonPressed = false;
volatile unsigned long lastLeftPress = 0;
volatile unsigned long lastRightPress = 0;

// --- BUTTON INTERRUPT HANDLERS ---
void IRAM_ATTR leftButtonISR() {
    unsigned long currentTime = millis();
    if (currentTime - lastLeftPress > 200) {
        leftButtonPressed = true;
        lastLeftPress = currentTime;
    }
}

void IRAM_ATTR rightButtonISR() {
    unsigned long currentTime = millis();
    if (currentTime - lastRightPress > 200) {
        rightButtonPressed = true;
        lastRightPress = currentTime;
    }
}

// --- HELPER FUNCTIONS ---
float readUltrasonic(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH, 30000);
    if (duration == 0) return 999.0;
    return duration * 0.034 / 2;
}

uint32_t getTempColor(float temp) {
    if (!carState.ambientOn) return ambientLight.Color(0, 0, 0);
    if (temp <= 20) return ambientLight.Color(0, 0, 255); // Blue
    if (temp >= 35) return ambientLight.Color(255, 0, 0); // Red
    
    int r = map(constrain(temp, 20, 35), 20, 35, 0, 255);
    int g = map(constrain(temp, 20, 35), 20, 35, 255, 0);
    return ambientLight.Color(r, g, 0);
}

// --- HTTP COMMUNICATION WITH CAR 2 ---
void sendDataToCar2() {
    if (!car2Connected || car2IP.length() == 0) return;
    
    HTTPClient http;
    http.begin("http://" + car2IP + "/update");
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<200> doc;
    doc["leftIndicator"] = carState.leftIndicator;
    doc["rightIndicator"] = carState.rightIndicator;
    doc["buzzerOn"] = carState.buzzerOn;
    doc["ambientOn"] = carState.ambientOn;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpResponseCode = http.POST(jsonString);
    if (httpResponseCode > 0) {
        String response = http.getString();
        StaticJsonDocument<200> responseDoc;
        deserializeJson(responseDoc, response);
        carState.car2Left = responseDoc["leftIndicator"] | false;
        carState.car2Right = responseDoc["rightIndicator"] | false;
    }
    http.end();
}

void checkCar2Status() {
    car2Connected = false;
    car2IP = "";
    
    wifi_sta_list_t wifi_sta_list;
    tcpip_adapter_sta_list_t adapter_sta_list;
    
    if (esp_wifi_ap_get_sta_list(&wifi_sta_list) == ESP_OK) {
        tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
        
        for (int i = 0; i < adapter_sta_list.num; i++) {
            tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
            String ip = IPAddress(station.ip.addr).toString();
            
            HTTPClient http;
            http.begin("http://" + ip + "/status");
            http.setTimeout(1000);
            
            int httpResponseCode = http.GET();
            if (httpResponseCode == 200) {
                String response = http.getString();
                if (response.indexOf("car2") != -1) {
                    car2IP = ip;
                    car2Connected = true;
                    
                    StaticJsonDocument<200> doc;
                    deserializeJson(doc, response);
                    carState.car2Left = doc["leftIndicator"] | false;
                    carState.car2Right = doc["rightIndicator"] | false;
                    
                    Serial.println("Car 2 found at: " + ip);
                    break;
                }
            }
            http.end();
        }
    }
}

// --- WEBSOCKET HANDLER ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            StaticJsonDocument<200> doc;
            deserializeJson(doc, data, len);
            
            if (doc.containsKey("action")) {
                String action = doc["action"];
                if (action == "left_indicator") {
                    carState.leftIndicator = !carState.leftIndicator;
                    if (carState.leftIndicator) carState.rightIndicator = false;
                } else if (action == "right_indicator") {
                    carState.rightIndicator = !carState.rightIndicator;
                    if (carState.rightIndicator) carState.leftIndicator = false;
                } else if (action == "buzzer_toggle") {
                    carState.buzzerOn = doc["value"];
                } else if (action == "ambient_toggle") {
                    carState.ambientOn = doc["value"];
                }
            }
        }
    }
}

// =================================================================
//                      SETUP
// =================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting Smart Car - Car 1");

    // Initialize Pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LEFT_LED, OUTPUT);
    pinMode(RIGHT_LED, OUTPUT);
    pinMode(LEFT_BUTTON, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON, INPUT_PULLUP);
    pinMode(FRONT_TRIG, OUTPUT);
    pinMode(FRONT_ECHO, INPUT);
    pinMode(BACK_TRIG, OUTPUT);
    pinMode(BACK_ECHO, INPUT);

    // Attach Interrupts
    attachInterrupt(digitalPinToInterrupt(LEFT_BUTTON), leftButtonISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(RIGHT_BUTTON), rightButtonISR, FALLING);

    // Initialize Sensors
    dht.begin();
    Serial.println("DHT11 initialized");

    ambientLight.begin();
    ambientLight.setBrightness(50);
    ambientLight.clear();
    ambientLight.show();
    Serial.println("NeoPixel initialized");

    Wire.begin();
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
    } else {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Serial.println("MPU6050 initialized");
    }

    // Initialize WiFi AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // Initialize Web Server
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", WEB_PAGE);
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["type"] = "car1";
        doc["leftIndicator"] = carState.leftIndicator;
        doc["rightIndicator"] = carState.rightIndicator;
        doc["temp"] = carState.temp;
        doc["humidity"] = carState.humidity;
        doc["frontDist"] = carState.frontDist;
        doc["backDist"] = carState.backDist;
        
        String jsonString;
        serializeJson(doc, jsonString);
        request->send(200, "application/json", jsonString);
    });

    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<200> doc;
            deserializeJson(doc, data, len);
            
            if (doc.containsKey("leftIndicator")) {
                carState.car2Left = doc["leftIndicator"];
                if (carState.car2Left) carState.car2Right = false;
            }
            if (doc.containsKey("rightIndicator")) {
                carState.car2Right = doc["rightIndicator"];
                if (carState.car2Right) carState.car2Left = false;
            }
            if (doc.containsKey("buzzerOn")) {
                carState.buzzerOn = doc["buzzerOn"];
            }
            if (doc.containsKey("ambientOn")) {
                carState.ambientOn = doc["ambientOn"];
            }
            
            StaticJsonDocument<200> responseDoc;
            responseDoc["leftIndicator"] = carState.leftIndicator;
            responseDoc["rightIndicator"] = carState.rightIndicator;
            String response;
            serializeJson(responseDoc, response);
            request->send(200, "application/json", response);
        });

    server.begin();
    Serial.println("HTTP server started");

    // LED test
    digitalWrite(LEFT_LED, HIGH);
    digitalWrite(RIGHT_LED, HIGH);
    delay(500);
    digitalWrite(LEFT_LED, LOW);
    digitalWrite(RIGHT_LED, LOW);
    
    Serial.println("Setup complete");
}

// =================================================================
//                       MAIN LOOP
// =================================================================
void loop() {
    ws.cleanupClients();
    unsigned long currentTime = millis();

    // Handle Button Presses
    if (leftButtonPressed) {
        leftButtonPressed = false;
        carState.leftIndicator = !carState.leftIndicator;
        if (carState.leftIndicator) carState.rightIndicator = false;
        Serial.println("Left indicator: " + String(carState.leftIndicator ? "ON" : "OFF"));
    }

    if (rightButtonPressed) {
        rightButtonPressed = false;
        carState.rightIndicator = !carState.rightIndicator;
        if (carState.rightIndicator) carState.leftIndicator = false;
        Serial.println("Right indicator: " + String(carState.rightIndicator ? "ON" : "OFF"));
    }

    // Read Sensors (every 250ms)
    if (currentTime - lastSensorRead > 250) {
        lastSensorRead = currentTime;

        // DHT11
        float newTemp = dht.readTemperature();
        float newHumidity = dht.readHumidity();
        if (!isnan(newTemp) && !isnan(newHumidity)) {
            carState.temp = newTemp;
            carState.humidity = newHumidity;
        }

        // Ultrasonic sensors
        carState.frontDist = readUltrasonic(FRONT_TRIG, FRONT_ECHO);
        carState.backDist = readUltrasonic(BACK_TRIG, BACK_ECHO);

        // MPU6050
        sensors_event_t a, g, temp;
        if (mpu.getEvent(&a, &g, &temp)) {
            float totalAccel = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
            carState.speed = (abs(totalAccel - 9.8) > 2.0) ? map(constrain(abs(totalAccel - 9.8), 2, 20), 2, 20, 1, 99) : 0;
            
            compassDirection -= g.gyro.z * 0.25;
            while (compassDirection > 360) compassDirection -= 360;
            while (compassDirection < 0) compassDirection += 360;
            carState.direction = compassDirection;
        }
    }

    // Indicator Blinking (every 500ms)
    if (currentTime - lastIndicatorBlink > 500) {
        lastIndicatorBlink = currentTime;
        indicatorState = !indicatorState;
    }

    // Control Outputs
    bool leftOn = (carState.leftIndicator || carState.car2Left) && indicatorState;
    bool rightOn = (carState.rightIndicator || carState.car2Right) && indicatorState;
    
    digitalWrite(LEFT_LED, leftOn ? HIGH : LOW);
    digitalWrite(RIGHT_LED, rightOn ? HIGH : LOW);

    // Buzzer
    bool isIndicatorOn = carState.leftIndicator || carState.rightIndicator || carState.car2Left || carState.car2Right;
    bool obstacleAlert = (carState.frontDist < 12.0 || carState.backDist < 12.0);
    
    if (carState.buzzerOn && ((isIndicatorOn && indicatorState) || obstacleAlert)) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else {
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Ambient light
    if (carState.ambientOn) {
        if (obstacleAlert) {
            uint32_t color = indicatorState ? ambientLight.Color(255, 0, 0) : ambientLight.Color(0, 0, 0);
            ambientLight.setPixelColor(0, color);
        } else {
            ambientLight.setPixelColor(0, getTempColor(carState.temp));
        }
    } else {
        ambientLight.setPixelColor(0, ambientLight.Color(0, 0, 0));
    }
    ambientLight.show();

    // Check Car 2 Status (every 2 seconds)
    if (currentTime - lastCar2Check > 2000) {
        lastCar2Check = currentTime;
        checkCar2Status();
    }

    // Send Data to Car 2 (every 500ms)
    if (currentTime - lastCar2Send > 500) {
        lastCar2Send = currentTime;
        sendDataToCar2();
    }

    // Send WebSocket Data (every 100ms)
    if (currentTime - lastWsSend > 100) {
        lastWsSend = currentTime;
        StaticJsonDocument<200> doc;
        doc["temp"] = carState.temp;
        doc["humidity"] = carState.humidity;
        doc["frontDist"] = carState.frontDist;
        doc["backDist"] = carState.backDist;
        doc["speed"] = carState.speed;
        doc["direction"] = carState.direction;
        doc["leftIndicator"] = carState.leftIndicator;
        doc["rightIndicator"] = carState.rightIndicator;
        doc["buzzerOn"] = carState.buzzerOn;
        doc["ambientOn"] = carState.ambientOn;
        doc["car2Connected"] = car2Connected;
        doc["car2Left"] = carState.car2Left;
        doc["car2Right"] = carState.car2Right;
        
        String jsonString;
        serializeJson(doc, jsonString);
        ws.textAll(jsonString);
    }
}
