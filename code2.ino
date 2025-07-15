#include <WiFi.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <MPU6050.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Pin definitions
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define BUZZER_PIN 5
#define NEOPIXEL_PIN 2
#define NEOPIXEL_COUNT 1
#define LEFT_LED 18
#define RIGHT_LED 19
#define CONTROL_BUTTON 32
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
WiFiManager wifiManager;

// Global variables
bool leftIndicator1 = false;
bool rightIndicator1 = false;
bool leftIndicator2 = false;
bool rightIndicator2 = false;
bool obstacleAlert = false;
String obstacleLocation = "Clear";
float temperature = 0;
float humidity = 0;
int frontDistance = 0;
int backDistance = 0;
float speed = 0;
float direction = 0;
unsigned long lastIndicatorToggle = 0;
unsigned long lastSensorRead = 0;
unsigned long lastDHTRead = 0;
unsigned long lastMPURead = 0;
unsigned long lastBuzzTime = 0;
int buzzInterval = 500;
float accelThreshold = 0.15; // Acceleration threshold for movement detection

// Button state
unsigned long lastButtonPress = 0;
int pressCount = 0;
bool buttonProcessed = true;

// ESP-NOW setup
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast address

// Message structure
typedef struct {
  uint8_t carId;
  bool leftIndicator;
  bool rightIndicator;
} IndicatorMessage;

// HTML page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Car Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
        }
        
        body {
            background: #0a0e17;
            color: #e0e0e0;
            overflow: hidden;
            height: 100vh;
            display: flex;
            flex-direction: column;
        }
        
        .header {
            padding: 15px;
            background: rgba(20, 25, 35, 0.9);
            border-bottom: 1px solid #2a3a5a;
            display: flex;
            justify-content: space-between;
            align-items: center;
            z-index: 10;
        }
        
        .logo {
            font-weight: 700;
            font-size: 1.2rem;
            color: #4dabf5;
        }
        
        .wifi-status {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 0.9rem;
        }
        
        .map-container {
            flex: 1;
            position: relative;
            background: linear-gradient(135deg, #1a2a4f 0%, #0d1529 100%);
            overflow: hidden;
        }
        
        .map-grid {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-image: 
                linear-gradient(90deg, rgba(40, 60, 100, 0.1) 1px, transparent 1px),
                linear-gradient(rgba(40, 60, 100, 0.1) 1px, transparent 1px);
            background-size: 50px 50px;
        }
        
        .road {
            position: absolute;
            top: 50%;
            left: 0;
            right: 0;
            height: 80px;
            background: #2a3a5a;
            transform: translateY(-50%);
            display: flex;
            justify-content: center;
            align-items: center;
        }
        
        .road::before {
            content: '';
            position: absolute;
            top: 50%;
            left: 0;
            right: 0;
            height: 4px;
            background: repeating-linear-gradient(
                90deg,
                #ffd700 0px,
                #ffd700 30px,
                transparent 30px,
                transparent 60px
            );
            transform: translateY(-50%);
        }
        
        .car-container {
            position: absolute;
            top: 50%;
            transform: translateY(-50%);
            z-index: 20;
        }
        
        .car1 {
            left: 35%;
        }
        
        .car2 {
            left: 65%;
        }
        
        .car-icon {
            width: 60px;
            height: 30px;
            background: #4dabf5;
            border-radius: 8px 8px 4px 4px;
            position: relative;
            transform: rotate(90deg);
        }
        
        .car-icon::before {
            content: '';
            position: absolute;
            top: -8px;
            left: 10px;
            width: 40px;
            height: 12px;
            background: #4dabf5;
            border-radius: 8px 8px 0 0;
        }
        
        .indicator {
            position: absolute;
            width: 8px;
            height: 8px;
            background: #ff9e00;
            border-radius: 50%;
            opacity: 0;
        }
        
        .indicator.left {
            left: -12px;
            top: 50%;
            transform: translateY(-50%);
        }
        
        .indicator.right {
            right: -12px;
            top: 50%;
            transform: translateY(-50%);
        }
        
        .indicator.active {
            opacity: 1;
            animation: blink 1s infinite;
        }
        
        @keyframes blink {
            0%, 50% { opacity: 1; }
            51%, 100% { opacity: 0.3; }
        }
        
        .direction-display {
            position: absolute;
            top: 20px;
            left: 20px;
            display: flex;
            align-items: center;
            gap: 12px;
        }
        
        .compass {
            width: 70px;
            height: 70px;
            background: rgba(20, 25, 35, 0.8);
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            border: 2px solid #3a4a6a;
            position: relative;
        }
        
        .compass-rose {
            position: absolute;
            width: 90%;
            height: 90%;
            background: rgba(30, 40, 60, 0.6);
            border-radius: 50%;
        }
        
        .compass-point {
            position: absolute;
            color: #a0b0d0;
            font-weight: 700;
            font-size: 12px;
        }
        
        .compass-point.n { top: 5px; left: 50%; transform: translateX(-50%); }
        .compass-point.e { top: 50%; right: 5px; transform: translateY(-50%); }
        .compass-point.s { bottom: 5px; left: 50%; transform: translateX(-50%); }
        .compass-point.w { top: 50%; left: 5px; transform: translateY(-50%); }
        
        .compass-needle {
            position: relative;
            width: 3px;
            height: 30px;
            background: #ff6b6b;
            border-radius: 2px;
            transform-origin: center center;
            transition: transform 0.5s;
            z-index: 2;
        }
        
        .compass-needle::after {
            content: '';
            position: absolute;
            bottom: -1px;
            left: -5px;
            width: 13px;
            height: 13px;
            background: #ff6b6b;
            border-radius: 50%;
        }
        
        .direction-text {
            font-size: 1.2rem;
            font-weight: 500;
            color: #e0e0e0;
            min-width: 80px;
        }
        
        .speed-display {
            position: absolute;
            top: 20px;
            right: 20px;
            background: rgba(20, 25, 35, 0.8);
            padding: 12px 20px;
            border-radius: 20px;
            font-size: 1.4rem;
            font-weight: 700;
            text-align: center;
            border: 2px solid #3a4a6a;
            min-width: 120px;
        }
        
        .speed-value {
            color: #4dabf5;
            font-size: 2rem;
            margin-top: 5px;
        }
        
        .speed-label {
            font-size: 0.9rem;
            color: #a0b0d0;
            margin-top: 2px;
        }
        
        .alert {
            position: absolute;
            bottom: 30px;
            left: 50%;
            transform: translateX(-50%);
            padding: 15px 30px;
            background: rgba(220, 53, 69, 0.9);
            color: white;
            border-radius: 30px;
            text-align: center;
            font-weight: 500;
            opacity: 0;
            transition: all 0.3s ease;
            z-index: 5;
        }
        
        .alert.show {
            opacity: 1;
            transform: translateX(-50%) translateY(0);
        }
        
        .controls-dock {
            background: rgba(20, 25, 35, 0.95);
            padding: 20px;
            border-top: 1px solid #2a3a5a;
            z-index: 100;
        }
        
        .dock-header {
            display: flex;
            justify-content: space-between;
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 1px solid #2a3a5a;
        }
        
        .dock-title {
            font-weight: 500;
            color: #4dabf5;
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        
        .status-card {
            background: rgba(40, 50, 70, 0.5);
            padding: 15px;
            border-radius: 12px;
            text-align: center;
            border: 1px solid rgba(90, 120, 180, 0.3);
        }
        
        .status-card.alert {
            background: rgba(200, 50, 50, 0.25);
            border: 1px solid rgba(220, 80, 80, 0.5);
        }
        
        .status-value {
            font-size: 1.4rem;
            font-weight: 700;
            margin-bottom: 5px;
        }
        
        .status-card .status-value {
            color: #4dabf5;
        }
        
        .status-card.alert .status-value {
            color: #ff6b6b;
        }
        
        .status-label {
            font-size: 0.85rem;
            color: #a0b0d0;
        }
        
        .status-distance {
            font-size: 0.8rem;
            color: #ff9e00;
            margin-top: 3px;
        }
        
        .controls-row {
            display: flex;
            justify-content: center;
            gap: 20px;
            margin-top: 10px;
        }
        
        .control-btn {
            flex: 1;
            max-width: 200px;
            padding: 18px;
            background: rgba(40, 50, 70, 0.7);
            color: #e0e0e0;
            border: none;
            border-radius: 12px;
            font-size: 1.1rem;
            font-weight: 500;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }
        
        .control-btn:active {
            transform: scale(0.96);
        }
        
        .control-btn.active {
            background: rgba(77, 171, 245, 0.2);
            border: 2px solid #4dabf5;
            color: #4dabf5;
        }
        
        .car-selector {
            display: flex;
            justify-content: center;
            gap: 10px;
            margin-bottom: 15px;
        }
        
        .car-btn {
            padding: 10px 20px;
            background: rgba(40, 50, 70, 0.7);
            border: none;
            border-radius: 8px;
            color: #e0e0e0;
            cursor: pointer;
        }
        
        .car-btn.active {
            background: rgba(77, 171, 245, 0.3);
            border: 1px solid #4dabf5;
        }
        
        @media (max-width: 768px) {
            .direction-display {
                top: 15px;
                left: 15px;
            }
            
            .speed-display {
                top: 15px;
                right: 15px;
                padding: 10px 15px;
                min-width: 100px;
            }
            
            .speed-value {
                font-size: 1.7rem;
            }
            
            .compass {
                width: 60px;
                height: 60px;
            }
            
            .status-grid {
                grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
                gap: 10px;
            }
            
            .controls-row {
                gap: 10px;
            }
            
            .control-btn {
                padding: 15px;
                font-size: 1rem;
            }
            
            .car1 {
                left: 30%;
            }
            
            .car2 {
                left: 70%;
            }
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="logo">SMART CAR DASHBOARD</div>
        <div class="wifi-status">
            <span id="wifiStatus">Connected</span>
        </div>
    </div>
    
    <div class="map-container">
        <div class="map-grid"></div>
        
        <div class="road">
            <div class="car-container car1">
                <div class="car-icon">
                    <div class="indicator left" id="leftIndicator1"></div>
                    <div class="indicator right" id="rightIndicator1"></div>
                </div>
            </div>
            <div class="car-container car2">
                <div class="car-icon">
                    <div class="indicator left" id="leftIndicator2"></div>
                    <div class="indicator right" id="rightIndicator2"></div>
                </div>
            </div>
        </div>
        
        <div class="direction-display">
            <div class="compass">
                <div class="compass-rose"></div>
                <div class="compass-point n">N</div>
                <div class="compass-point e">E</div>
                <div class="compass-point s">S</div>
                <div class="compass-point w">W</div>
                <div class="compass-needle" id="compassNeedle"></div>
            </div>
            <div class="direction-text">
                <div>Heading</div>
                <div id="headingText">0°</div>
            </div>
        </div>
        
        <div class="speed-display">
            <div>SPEED</div>
            <div class="speed-value" id="speedDisplay">0</div>
            <div class="speed-label">km/h</div>
        </div>
        
        <div class="alert" id="alertBox">
            <span id="alertText">Obstacle Detected!</span>
        </div>
    </div>
    
    <div class="controls-dock">
        <div class="dock-header">
            <div class="dock-title">CAR STATUS</div>
        </div>
        
        <div class="car-selector">
            <button class="car-btn active" id="car1Btn" onclick="selectCar(1)">Car 1</button>
            <button class="car-btn" id="car2Btn" onclick="selectCar(2)">Car 2</button>
        </div>
        
        <div class="status-grid">
            <div class="status-card">
                <div class="status-value" id="temperature">--</div>
                <div class="status-label">Temperature</div>
            </div>
            <div class="status-card">
                <div class="status-value" id="humidity">--</div>
                <div class="status-label">Humidity</div>
            </div>
            <div class="status-card" id="proximityCard">
                <div class="status-value" id="proximityStatus">Clear</div>
                <div class="status-label">Proximity</div>
                <div class="status-distance" id="proximityDistance"></div>
            </div>
        </div>
        
        <div class="controls-row">
            <button class="control-btn" id="leftBtn" onclick="toggleIndicator('left')">
                <span>←</span> LEFT
            </button>
            <button class="control-btn" id="rightBtn" onclick="toggleIndicator('right')">
                RIGHT <span>→</span>
            </button>
        </div>
    </div>
    
    <script>
        let selectedCar = 1;
        let lastPress = 0;
        let pressCount = 0;
        let pressTimeout;
        
        // Select active car
        function selectCar(carNum) {
            selectedCar = carNum;
            document.getElementById('car1Btn').classList.toggle('active', carNum === 1);
            document.getElementById('car2Btn').classList.toggle('active', carNum === 2);
        }
        
        // Toggle indicators
        function toggleIndicator(side) {
            fetch(`/api/indicator/${selectedCar}/${side}`, { method: 'POST' });
        }
        
        // Update dashboard status
        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    // Update temperature and humidity
                    document.getElementById('temperature').textContent = 
                        data.temperature === -999 ? '--' : data.temperature.toFixed(1);
                    document.getElementById('humidity').textContent = 
                        data.humidity === -999 ? '--' : data.humidity.toFixed(0);
                    
                    // Update proximity status
                    const proximityCard = document.getElementById('proximityCard');
                    const proximityStatus = document.getElementById('proximityStatus');
                    const proximityDistance = document.getElementById('proximityDistance');
                    
                    proximityStatus.textContent = data.obstacleLocation;
                    proximityDistance.textContent = '';
                    
                    if (data.obstacleAlert) {
                        proximityCard.classList.add('alert');
                        if (data.frontDistance < 10) {
                            proximityDistance.textContent += `Front: ${data.frontDistance}cm `;
                        }
                        if (data.backDistance < 10) {
                            proximityDistance.textContent += `Back: ${data.backDistance}cm`;
                        }
                    } else {
                        proximityCard.classList.remove('alert');
                    }
                    
                    // Update speed
                    document.getElementById('speedDisplay').textContent = data.speed.toFixed(0);
                    
                    // Update compass and heading
                    document.getElementById('compassNeedle').style.transform = 
                        `rotate(${data.direction}deg)`;
                    document.getElementById('headingText').textContent = 
                        `${Math.round(data.direction)}°`;
                    
                    // Update indicators for both cars
                    document.getElementById('leftIndicator1').classList.toggle('active', data.leftIndicator1);
                    document.getElementById('rightIndicator1').classList.toggle('active', data.rightIndicator1);
                    document.getElementById('leftIndicator2').classList.toggle('active', data.leftIndicator2);
                    document.getElementById('rightIndicator2').classList.toggle('active', data.rightIndicator2);
                    
                    // Update button states
                    document.getElementById('leftBtn').classList.toggle('active', 
                        (selectedCar === 1 && data.leftIndicator1) || (selectedCar === 2 && data.leftIndicator2));
                    document.getElementById('rightBtn').classList.toggle('active', 
                        (selectedCar === 1 && data.rightIndicator1) || (selectedCar === 2 && data.rightIndicator2));
                    
                    // Update obstacle alert
                    const alertBox = document.getElementById('alertBox');
                    const alertText = document.getElementById('alertText');
                    if (data.obstacleAlert) {
                        alertText.textContent = `OBSTACLE DETECTED: ${data.obstacleLocation}`;
                        alertBox.classList.add('show');
                    } else {
                        alertBox.classList.remove('show');
                    }
                })
                .catch(error => console.error('Error:', error));
        }
        
        // Update status every 100ms for real-time feel
        setInterval(updateStatus, 100);
        
        // Initial status update
        updateStatus();
    </script>
</body>
</html>
)rawliteral";

// ESP-NOW callback
void onDataReceived(const uint8_t * mac, const uint8_t *incomingData, int len) {
  IndicatorMessage* msg = (IndicatorMessage*)incomingData;
  
  if (msg->carId == 1) {
    leftIndicator1 = msg->leftIndicator;
    rightIndicator1 = msg->rightIndicator;
    Serial.println("Received Car1 indicators: " + 
                  String(leftIndicator1 ? "LEFT" : "") + 
                  String(rightIndicator1 ? "RIGHT" : ""));
  } 
  else if (msg->carId == 2) {
    leftIndicator2 = msg->leftIndicator;
    rightIndicator2 = msg->rightIndicator;
    Serial.println("Received Car2 indicators: " + 
                  String(leftIndicator2 ? "LEFT" : "") + 
                  String(rightIndicator2 ? "RIGHT" : ""));
  }
}

void sendIndicatorSync(uint8_t carId, bool left, bool right) {
  IndicatorMessage msg;
  msg.carId = carId;
  msg.leftIndicator = left;
  msg.rightIndicator = right;
  
  esp_now_send(broadcastAddress, (uint8_t *) &msg, sizeof(msg));
}

void toggleIndicator(uint8_t carId, char side) {
  if (carId == 1) {
    if (side == 'left') {
      leftIndicator1 = !leftIndicator1;
      if (leftIndicator1) rightIndicator1 = false;
    } 
    else if (side == 'right') {
      rightIndicator1 = !rightIndicator1;
      if (rightIndicator1) leftIndicator1 = false;
    }
    sendIndicatorSync(1, leftIndicator1, rightIndicator1);
  } 
  else if (carId == 2) {
    if (side == 'left') {
      leftIndicator2 = !leftIndicator2;
      if (leftIndicator2) rightIndicator2 = false;
    } 
    else if (side == 'right') {
      rightIndicator2 = !rightIndicator2;
      if (rightIndicator2) leftIndicator2 = false;
    }
    sendIndicatorSync(2, leftIndicator2, rightIndicator2);
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Initialize MPU6050
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
  } else {
    Serial.println("MPU6050 connected");
  }
  
  // Initialize pins
  pinMode(LEFT_LED, OUTPUT);
  pinMode(RIGHT_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CONTROL_BUTTON, INPUT_PULLUP);
  pinMode(FRONT_TRIG, OUTPUT);
  pinMode(FRONT_ECHO, INPUT);
  pinMode(BACK_TRIG, OUTPUT);
  pinMode(BACK_ECHO, INPUT);
  
  // Initialize components
  dht.begin();
  strip.begin();
  strip.show();
  
  // Initialize WiFiManager
  WiFi.mode(WIFI_STA);
  wifiManager.autoConnect("SmartCarAP");
  Serial.println("WiFi connected!");
  Serial.println("IP address: " + WiFi.localIP().toString());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Set up broadcast peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }
  
  // Register callback
  esp_now_register_recv_cb(onDataReceived);
  
  // Setup web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });
  
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request){
    StaticJsonDocument<500> doc;
    doc["temperature"] = isnan(temperature) ? -999 : temperature;
    doc["humidity"] = isnan(humidity) ? -999 : humidity;
    doc["leftIndicator1"] = leftIndicator1;
    doc["rightIndicator1"] = rightIndicator1;
    doc["leftIndicator2"] = leftIndicator2;
    doc["rightIndicator2"] = rightIndicator2;
    doc["obstacleAlert"] = obstacleAlert;
    doc["obstacleLocation"] = obstacleLocation;
    doc["frontDistance"] = frontDistance;
    doc["backDistance"] = backDistance;
    doc["speed"] = speed;
    doc["direction"] = direction;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  server.on("/api/indicator/1/left", HTTP_POST, [](AsyncWebServerRequest *request){
    toggleIndicator(1, 'left');
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  server.on("/api/indicator/1/right", HTTP_POST, [](AsyncWebServerRequest *request){
    toggleIndicator(1, 'right');
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  server.on("/api/indicator/2/left", HTTP_POST, [](AsyncWebServerRequest *request){
    toggleIndicator(2, 'left');
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  server.on("/api/indicator/2/right", HTTP_POST, [](AsyncWebServerRequest *request){
    toggleIndicator(2, 'right');
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });
  
  // Start server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Handle button input
  handleButton();
  
  // Read sensors every 50ms
  if (currentTime - lastSensorRead > 50) {
    readUltrasonicSensors();
    lastSensorRead = currentTime;
  }
  
  // Read DHT every 1 second
  if (currentTime - lastDHTRead > 1000) {
    readDHTSensor();
    lastDHTRead = currentTime;
  }
  
  // Read MPU6050 every 100ms
  if (currentTime - lastMPURead > 100) {
    readMPU6050();
    lastMPURead = currentTime;
  }
  
  // Handle turn indicator blinking
  if (currentTime - lastIndicatorToggle > 500) {
    handleIndicatorBlinking();
    lastIndicatorToggle = currentTime;
  }
  
  // Handle obstacle alerts
  handleObstacleAlert();
  
  // Handle ambient lighting
  handleAmbientLighting();
  
  delay(10);
}

void handleButton() {
  if (digitalRead(CONTROL_BUTTON) == LOW && buttonProcessed) {
    buttonProcessed = false;
    unsigned long pressTime = millis();
    
    // Wait for button release with timeout
    while (digitalRead(CONTROL_BUTTON) == LOW) {
      if (millis() - pressTime > 1000) break; // Safety timeout
      delay(10);
    }
    
    // Count press duration
    unsigned long duration = millis() - pressTime;
    
    if (duration < 500) { // Short press
      if (millis() - lastButtonPress < 300) {
        // Double press detected
        toggleIndicator(1, 'right');
        pressCount = 0;
      } else {
        // Start single press timeout
        pressCount = 1;
      }
      lastButtonPress = millis();
    }
  } 
  else if (digitalRead(CONTROL_BUTTON) == HIGH) {
    buttonProcessed = true;
  }
  
  // Handle single press after timeout
  if (pressCount == 1 && millis() - lastButtonPress > 300) {
    toggleIndicator(1, 'left');
    pressCount = 0;
  }
}

void readUltrasonicSensors() {
  // Read front sensor
  digitalWrite(FRONT_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(FRONT_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(FRONT_TRIG, LOW);
  long frontDuration = pulseIn(FRONT_ECHO, HIGH, 30000);
  frontDistance = frontDuration > 0 ? frontDuration * 0.034 / 2 : 999;
  
  // Read back sensor
  digitalWrite(BACK_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(BACK_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(BACK_TRIG, LOW);
  long backDuration = pulseIn(BACK_ECHO, HIGH, 30000);
  backDistance = backDuration > 0 ? backDuration * 0.034 / 2 : 999;
  
  // Check for obstacles within 10cm
  bool frontObstacle = (frontDistance < 10 && frontDistance > 0);
  bool backObstacle = (backDistance < 10 && backDistance > 0);
  
  obstacleAlert = frontObstacle || backObstacle;
  
  if (frontObstacle && backObstacle) {
    obstacleLocation = "Front & Back";
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
  }
}

void readMPU6050() {
  if (mpu.testConnection()) {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    // Calculate direction from gyroscope Z-axis
    direction += (gz / 131.0) * 0.1;
    if (direction > 360) direction -= 360;
    if (direction < 0) direction += 360;
    
    // Calculate acceleration magnitude
    float accelMagnitude = sqrt(ax*ax + ay*ay + az*az) / 16384.0;
    
    // Calculate speed - only 0,1,2
    if (accelMagnitude > accelThreshold) {
      speed = (accelMagnitude > accelThreshold * 2) ? 2.0 : 1.0;
    } else {
      speed = 0.0;
    }
  }
}

void handleIndicatorBlinking() {
  static bool blinkState = false;
  blinkState = !blinkState;
  
  // Handle Car 1 indicators
  digitalWrite(LEFT_LED, blinkState && leftIndicator1);
  digitalWrite(RIGHT_LED, blinkState && rightIndicator1);
  
  // Sound for indicators
  if ((leftIndicator1 || rightIndicator1) && blinkState) {
    tone(BUZZER_PIN, 1200, 100);
  }
}

void handleObstacleAlert() {
  unsigned long currentTime = millis();
  
  if (obstacleAlert) {
    // Flash neopixel red
    strip.setPixelColor(0, strip.Color(255, 0, 0));
    strip.show();
    
    // Calculate closest distance
    int minDistance = min(frontDistance, backDistance);
    
    // Map distance to beep frequency
    buzzInterval = map(minDistance, 1, 10, 100, 500);
    
    // Sound buzzer
    if (currentTime - lastBuzzTime > buzzInterval) {
      tone(BUZZER_PIN, 3000, 50);
      lastBuzzTime = currentTime;
    }
  }
}

void handleAmbientLighting() {
  if (!obstacleAlert) {
    // Set ambient color based on temperature
    if (temperature < 20 && !isnan(temperature)) {
      strip.setPixelColor(0, strip.Color(0, 100, 255));
    } else if (temperature < 25 && !isnan(temperature)) {
      strip.setPixelColor(0, strip.Color(0, 255, 100));
    } else if (!isnan(temperature)) {
      strip.setPixelColor(0, strip.Color(255, 150, 0));
    } else {
      strip.setPixelColor(0, strip.Color(100, 100, 100));
    }
    strip.show();
  }
}
