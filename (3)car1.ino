// =================================================================
//                 SMART CAR PROJECT - CAR 1 (MAIN)
// =================================================================
// This code is for the main car, which hosts the web server and
// communicates with both the web dashboard and the second car.
//
// Required Libraries:
// - ESPAsyncWebServer: https://github.com/me-no-dev/ESPAsyncWebServer
// - AsyncTCP: https://github.com/me-no-dev/AsyncTCP
// - Adafruit NeoPixel: by Adafruit
// - DHT sensor library: by Adafruit
// - Adafruit Unified Sensor: by Adafruit
// - Adafruit MPU6050: by Adafruit
//
// Make sure to place the 'web.h' file in the same directory as this sketch.

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <esp_now.h>

#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "web.h" // Our web page HTML/CSS/JS

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
// MPU6050 uses default I2C pins (SDA=21, SCL=22)

// --- WIFI & WEB SERVER CONFIG ---
const char *ssid = "SmartCar_Dashboard";
const char *password = "12345678";
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --- SENSOR OBJECTS ---
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_NeoPixel ambientLight(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_MPU6050 mpu;

// --- STATE VARIABLES ---
// These variables hold the current state of the car
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
    bool car2Connected = false;
    bool car2Left = false;
    bool car2Right = false;
} carState;

// --- ESP-NOW COMMUNICATION ---
// This structure must match the sender's structure (Car 2)
typedef struct struct_message {
    bool leftIndicator;
    bool rightIndicator;
} struct_message;
struct_message incomingReadings;

// --- TIMING & CONTROL VARIABLES ---
unsigned long lastWsSend = 0;
unsigned long lastSensorRead = 0;
unsigned long lastIndicatorBlink = 0;
unsigned long lastCar2MessageTime = 0;

bool indicatorState = false; // For blinking LEDs

// --- BUTTON DEBOUNCING ---
const int debounceDelay = 50;
int lastLeftButtonState = HIGH;
int lastRightButtonState = HIGH;
unsigned long lastLeftDebounceTime = 0;
unsigned long lastRightDebounceTime = 0;

// =================================================================
//                  ESP-NOW CALLBACK
// =================================================================
// This function is called whenever data is received from Car 2
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
    carState.car2Left = incomingReadings.leftIndicator;
    carState.car2Right = incomingReadings.rightIndicator;
    carState.car2Connected = true;
    lastCar2MessageTime = millis();
}

// =================================================================
//                  HELPER FUNCTIONS
// =================================================================

// --- READ ULTRASONIC SENSOR ---
float readUltrasonic(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
    return duration * 0.034 / 2;
}

// --- NEOPIXEL COLOR MAPPING ---
// Maps a value from a range to a color (blue -> green -> red)
uint32_t getTempColor(float temp) {
    if (!carState.ambientOn) return ambientLight.Color(0, 0, 0);
    if (temp <= 20) return ambientLight.Color(0, 0, 255); // Blue
    if (temp >= 35) return ambientLight.Color(255, 0, 0); // Red
    // Map temperature from 20-35 to a green-to-red gradient
    int r = map(temp, 20, 35, 0, 255);
    int g = map(temp, 20, 35, 255, 0);
    return ambientLight.Color(r, g, 0);
}

// --- WEBSOCKET EVENT HANDLER ---
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            JsonDocument doc;
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
    Wire.begin();

    // --- Initialize Pins ---
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LEFT_LED, OUTPUT);
    pinMode(RIGHT_LED, OUTPUT);
    pinMode(LEFT_BUTTON, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON, INPUT_PULLUP);
    pinMode(FRONT_TRIG, OUTPUT);
    pinMode(FRONT_ECHO, INPUT);
    pinMode(BACK_TRIG, OUTPUT);
    pinMode(BACK_ECHO, INPUT);

    // --- Initialize Sensors ---
    dht.begin();
    ambientLight.begin();
    ambientLight.setBrightness(50);
    ambientLight.show();

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) { delay(10); }
    }
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    // --- Initialize WiFi AP ---
    WiFi.softAP(ssid, password);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // --- Initialize ESP-NOW ---
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataRecv);

    // --- Initialize Web Server & WebSockets ---
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", WEB_PAGE);
    });
    server.begin();
    Serial.println("HTTP server started");
}

// =================================================================
//                       MAIN LOOP
// =================================================================
void loop() {
    ws.cleanupClients(); // Handle disconnected clients

    // --- TIMED TASKS ---
    unsigned long currentTime = millis();

    // Task 1: Read sensors periodically (every 250ms)
    if (currentTime - lastSensorRead > 250) {
        lastSensorRead = currentTime;

        // Read DHT11
        carState.temp = dht.readTemperature();
        carState.humidity = dht.readHumidity();
        if (isnan(carState.temp) || isnan(carState.humidity)) {
            carState.temp = 0; carState.humidity = 0;
        }

        // Read Ultrasonic Sensors
        carState.frontDist = readUltrasonic(FRONT_TRIG, FRONT_ECHO);
        carState.backDist = readUltrasonic(BACK_TRIG, BACK_ECHO);

        // Read MPU6050 and simulate speed/direction
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        
        // Simulate speed based on movement
        float totalAccel = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
        if (abs(totalAccel - 9.8) > 2.0) { // Detect motion beyond gravity
            carState.speed = 1;
        } else {
            carState.speed = 0;
        }

        // Simulate compass direction from Gyro Z-axis (Yaw)
        // This is a relative direction, not a true magnetic compass.
        carState.direction -= g.gyro.z * (250.0 / 1000.0); // Integrate over time
        if (carState.direction > 360) carState.direction -= 360;
        if (carState.direction < 0) carState.direction += 360;
    }

    // Task 2: Handle physical button presses with debouncing
    int leftReading = digitalRead(LEFT_BUTTON);
    if (leftReading != lastLeftButtonState) {
        lastLeftDebounceTime = currentTime;
    }
    if ((currentTime - lastLeftDebounceTime) > debounceDelay) {
        if (leftReading == LOW && lastLeftButtonState == HIGH) {
            carState.leftIndicator = !carState.leftIndicator;
            if (carState.leftIndicator) carState.rightIndicator = false;
        }
    }
    lastLeftButtonState = leftReading;

    int rightReading = digitalRead(RIGHT_BUTTON);
    if (rightReading != lastRightButtonState) {
        lastRightDebounceTime = currentTime;
    }
    if ((currentTime - lastRightDebounceTime) > debounceDelay) {
        if (rightReading == LOW && lastRightButtonState == HIGH) {
            carState.rightIndicator = !carState.rightIndicator;
            if (carState.rightIndicator) carState.leftIndicator = false;
        }
    }
    lastRightButtonState = rightReading;

    // Task 3: Handle blinking indicators, buzzer, and ambient light
    bool isIndicatorOn = carState.leftIndicator || carState.rightIndicator || (carState.car2Connected && (carState.car2Left || carState.car2Right));
    bool obstacleAlert = (carState.frontDist < 6.0 || carState.backDist < 6.0);

    if (currentTime - lastIndicatorBlink > 500) {
        lastIndicatorBlink = currentTime;
        indicatorState = !indicatorState; // Toggle blink state every 500ms
    }
    
    // Control Left LED: Blinks if our indicator is on, OR if car2's indicator is on
    digitalWrite(LEFT_LED, (carState.leftIndicator || (carState.car2Connected && carState.car2Left)) && indicatorState ? HIGH : LOW);
    
    // Control Right LED
    digitalWrite(RIGHT_LED, (carState.rightIndicator || (carState.car2Connected && carState.car2Right)) && indicatorState ? HIGH : LOW);

    // Control Buzzer: Beeps for indicators OR for close obstacles
    if (carState.buzzerOn && ( (isIndicatorOn && indicatorState) || obstacleAlert) ) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else {
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Control Ambient Light: Shows alert color for obstacles, otherwise shows temp color
    if (carState.ambientOn) {
        if (obstacleAlert) {
            ambientLight.setPixelColor(0, indicatorState ? ambientLight.Color(255, 0, 0) : ambientLight.Color(0, 0, 0)); // Flashing Red
        } else {
            ambientLight.setPixelColor(0, getTempColor(carState.temp));
        }
    } else {
        ambientLight.setPixelColor(0, ambientLight.Color(0, 0, 0)); // Off
    }
    ambientLight.show();


    // Task 4: Check for Car 2 connection timeout (if no message for 3s)
    if (carState.car2Connected && (currentTime - lastCar2MessageTime > 3000)) {
        carState.car2Connected = false;
        carState.car2Left = false;
        carState.car2Right = false;
    }

    // Task 5: Send data to WebSocket clients periodically (every 100ms)
    if (currentTime - lastWsSend > 100) {
        lastWsSend = currentTime;
        JsonDocument doc;
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
        doc["car2Connected"] = carState.car2Connected;
        doc["car2Left"] = carState.car2Left;
        doc["car2Right"] = carState.car2Right;
        
        String jsonString;
        serializeJson(doc, jsonString);
        ws.textAll(jsonString);
    }
}
