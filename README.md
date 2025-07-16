# IoT Based Smart Car Dashboard

We developed a project for a smart car dashboard using two ESP32 boards (Car 1 and Car 2) to display real-time sensor data and control indicators via a web interface. The system uses sensors to monitor temperature, humidity, obstacles, speed, and direction, with a visual dashboard showing cars, indicators, and obstacles.

---

## Project Overview

This project consists of two ESP32 microcontrollers:

- **Car 1:** The main car, hosting a WiFi access point and web server, equipped with:
  - MPU6050 for direction and speed,
  - Ultrasonic sensors for obstacle detection,
  - DHT11 for temperature and humidity.

- **Car 2:** A secondary car that connects to Car 1's WiFi, sharing indicator states and sensor data.

- **Web Interface:** A browser-based dashboard displaying sensor data, a compass, speedometer, two cars with blinking indicators, and dynamic obstacle icons.

---

## Files
- [Car1.ino](./(finalised)car1.ino) — Main ESP32 code managing sensors, indicators, and communication.
- [Car2.ino](./(finalised)car2.ino) — Secondary ESP32 code acting as a client to Car 1.
- [web.h](./(FINALISED)web.h) — HTML, CSS, and JavaScript for the web dashboard (upload it with CAR1.INO code)

--- 

### 1. Car1.ino

**Purpose:**  
Runs on the main ESP32 (Car 1), managing sensors, indicators, and communication.

**Functionality:**
- Reads data from:
  - DHT11: Temperature and humidity.
  - MPU6050: Yaw (direction) and acceleration (speed: 0, 1, or 2).
  - Ultrasonic Sensors: Front and back obstacle distances.
- Controls LEDs for left/right indicators and a buzzer for alerts.
- Hosts a WiFi access point (`SmartCar_Dashboard`, password: `12345678`).
- Runs a web server and WebSocket to update the dashboard.
- Communicates with Car 2 via HTTP to sync indicators and settings.

**Key Features:**
- Simplified yaw calculation for compass.
- Speed limited to 0 (no movement), 1 (minor), or 2 (significant).
- Buzzer alerts for obstacles (<6cm) or indicators.
- NeoPixel shows temperature-based colors or obstacle alerts.

---

### 2. Car2.ino

**Purpose:**  
Runs on the secondary ESP32 (Car 2), acting as a client to Car 1.

**Functionality:**
- Connects to Car 1’s WiFi network.
- Reads:
  - DHT11: Temperature and humidity.
  - Ultrasonic Sensors: Front and back obstacle distances.
- Uses a single button to toggle left (single press) or right (double press) indicators.
- Controls LEDs and buzzer, syncing with Car 1 via HTTP.

**Key Features:**
- Sends indicator states to Car 1.
- Buzzer activates for obstacles (<6cm) or indicators.
- Always displayed in the dashboard, even if disconnected.

---

### 3. web.h

**Purpose:**  
Contains the HTML, CSS, and JavaScript for the web-based dashboard.

**Functionality:**
- Displays:
  - Compass: Shows Car 1’s direction with a rotating arrow and N/S/E/W labels.
  - Speedometer: Shows speed (0, 1, or 2 MPH).
  - Two Cars: Car 1 (forward, red) and Car 2 (behind, smaller, gray), always visible.
  - Indicators: Blinking dots on both cars when turned on (Car 2 blinks only if connected).
  - Obstacles: Cone icon appears when obstacles are <6cm, positioned dynamically to avoid car overlap.
  - Sensor Data: Temperature, humidity, and nearest obstacle distance.
- Provides buttons to toggle indicators, buzzer, and ambient light.
- Uses WebSocket to receive real-time updates from Car 1.

**Key Features:**
- Enlarged compass and speedometer for visibility.
- Dynamic obstacle positioning based on distance.
- Responsive design for mobile devices.

---

## Workflow

### Setup:
- Car 1 starts a WiFi access point and web server.
- Car 2 connects to Car 1’s WiFi (optional, as Car 2 is shown in UI regardless).
- Sensors initialize (DHT11, MPU6050, ultrasonics).

### Operation:
- Car 1 reads sensors every 250ms, calculates direction (yaw) and speed, and checks for obstacles (<6cm).
- Car 2 reads sensors and syncs indicators with Car 1 via HTTP every 500ms.
- Buttons on both cars toggle indicators (left/right).
- Buzzer on both cars activates for obstacles or indicators.

### Dashboard:
- Access [http://192.168.4.1](http://192.168.4.1) on a device connected to `SmartCar_Dashboard`.
- WebSocket updates the UI every 100ms with sensor data, direction, speed, and indicator states.
- Obstacle cone appears in front or behind Car 1 if detected, moving closer as distance decreases.
- Car 2 is always shown; its indicators blink only if connected.

### Communication:
- Car 1 checks for Car 2 every 2 seconds.
- HTTP syncs indicator and buzzer states between cars.
- NeoPixel on Car 1 shows temperature colors or blinks red for obstacles.

---

## Hardware Requirements

### Car 1:
- ESP32 board
- DHT11 (Pin 4)
- MPU6050 (I2C)
- NeoPixel (Pin 2)
- Buzzer (Pin 5)
- LEDs (Pins 18, 19)
- Buttons (Pins 32, 33)
- Ultrasonic Sensors  
  - Front: TRIG 26, ECHO 27  
  - Back: TRIG 14, ECHO 12

### Car 2:
- ESP32 board
- DHT11 (Pin 4)
- Buzzer (Pin 5)
- LEDs (Pins 18, 19)
- Button (Pin 32)
- Ultrasonic Sensors  
  - Front: TRIG 26, ECHO 27  
  - Back: TRIG 14, ECHO 12

---

## Software Requirements

- Arduino IDE (I used Arduino IDE 2.3.6 and Arduino Cloud)

### Libraries:
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson (6.x or 7.x)
- Adafruit_NeoPixel
- DHT sensor library
- Adafruit_MPU6050
- Adafruit_Sensor
- HTTPClient

---

## Screenshot

![Smart Car Dashboard](https://github.com/user-attachments/assets/a9da8be7-8fe6-4ec8-8131-7bac42ed4307)

---
