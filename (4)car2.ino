
// =================================================================
//                 SMART CAR PROJECT - CAR 2
// =================================================================

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HTTPClient.h>

// --- PIN DEFINITIONS ---
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define BUZZER_PIN 5
#define LEFT_LED 18
#define RIGHT_LED 19
#define BUTTON_PIN 32
#define FRONT_TRIG 26
#define FRONT_ECHO 27
#define BACK_TRIG 14
#define BACK_ECHO 12

// --- WIFI CONFIG ---
const char *ssid = "SmartCar_Dashboard";
const char *password = "12345678";
AsyncWebServer server(80);

// --- SENSOR OBJECTS ---
DHT dht(DHT_PIN, DHT_TYPE);

// --- STATE VARIABLES ---
struct CarState {
    float temp = 0.0;
    float humidity = 0.0;
    float frontDist = 999.0;
    float backDist = 999.0;
    bool leftIndicator = false;
    bool rightIndicator = false;
    bool buzzerOn = true;
    bool ambientOn = true;
    bool car1Left = false;
    bool car1Right = false;
} carState;

// --- TIMING VARIABLES ---
unsigned long lastSensorRead = 0;
unsigned long lastIndicatorBlink = 0;
unsigned long lastCar1Send = 0;
bool indicatorState = false;

// --- BUTTON VARIABLES ---
volatile bool buttonPressed = false;
volatile unsigned long lastButtonPress = 0;
unsigned long lastButtonPressTime = 0;
bool waitingForSecondPress = false;
const unsigned long doublePressWindow = 300; // Time window for double press (ms)

// --- BUTTON INTERRUPT HANDLER ---
void IRAM_ATTR buttonISR() {
    unsigned long currentTime = millis();
    if (currentTime - lastButtonPress > 200) { // Debounce
        buttonPressed = true;
        lastButtonPress = currentTime;
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

// Send data to Car 1
void sendDataToCar1() {
    HTTPClient http;
    http.begin("http://192.168.4.1/update"); // Car 1 is the AP at 192.168.4.1
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
        carState.car1Left = responseDoc["leftIndicator"] | false;
        carState.car1Right = responseDoc["rightIndicator"] | false;
    }
    http.end();
}

// =================================================================
//                      SETUP
// =================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Starting Smart Car - Car 2");

    // Initialize Pins
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LEFT_LED, OUTPUT);
    pinMode(RIGHT_LED, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(FRONT_TRIG, OUTPUT);
    pinMode(FRONT_ECHO, INPUT);
    pinMode(BACK_TRIG, OUTPUT);
    pinMode(BACK_ECHO, INPUT);

    // Attach Interrupt
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    // Initialize Sensors
    dht.begin();
    Serial.println("DHT11 initialized");

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Initialize Web Server
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        StaticJsonDocument<200> doc;
        doc["type"] = "car2";
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
                carState.car1Left = doc["leftIndicator"];
                if (carState.car1Left) carState.car1Right = false;
            }
            if (doc.containsKey("rightIndicator")) {
                carState.car1Right = doc["rightIndicator"];
                if (carState.car1Right) carState.car1Left = false;
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
    unsigned long currentTime = millis();

    // Handle Button Presses
    if (buttonPressed) {
        buttonPressed = false;
        
        if (!waitingForSecondPress) {
            lastButtonPressTime = currentTime;
            waitingForSecondPress = true;
        } else if (currentTime - lastButtonPressTime < doublePressWindow) {
            // Double press: toggle right indicator
            carState.rightIndicator = !carState.rightIndicator;
            if (carState.rightIndicator) carState.leftIndicator = false;
            waitingForSecondPress = false;
            Serial.println("Right indicator: " + String(carState.rightIndicator ? "ON" : "OFF"));
        }
    }

    // Check if double press window has expired (single press)
    if (waitingForSecondPress && (currentTime - lastButtonPressTime > doublePressWindow)) {
        // Single press: toggle left indicator
        carState.leftIndicator = !carState.leftIndicator;
        if (carState.leftIndicator) carState.rightIndicator = false;
        waitingForSecondPress = false;
        Serial.println("Left indicator: " + String(carState.leftIndicator ? "ON" : "OFF"));
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
    }

    // Indicator Blinking (every 500ms)
    if (currentTime - lastIndicatorBlink > 500) {
        lastIndicatorBlink = currentTime;
        indicatorState = !indicatorState;
    }

    // Control Outputs
    bool leftOn = (carState.leftIndicator || carState.car1Left) && indicatorState;
    bool rightOn = (carState.rightIndicator || carState.car1Right) && indicatorState;
    
    digitalWrite(LEFT_LED, leftOn ? HIGH : LOW);
    digitalWrite(RIGHT_LED, rightOn ? HIGH : LOW);

    // Buzzer
    bool isIndicatorOn = carState.leftIndicator || carState.rightIndicator || carState.car1Left || carState.car1Right;
    bool obstacleAlert = (carState.frontDist < 12.0 || carState.backDist < 12.0);
    
    if (carState.buzzerOn && ((isIndicatorOn && indicatorState) || obstacleAlert)) {
        digitalWrite(BUZZER_PIN, HIGH);
    } else {
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Send Data to Car 1 (every 500ms)
    if (currentTime - lastCar1Send > 500) {
        lastCar1Send = currentTime;
        sendDataToCar1();
    }
}
