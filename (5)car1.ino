#pragma once

#include <pgmspace.h>

const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Smart Car Dashboard</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
        }

        :root {
            --bg-color: #1a1a1a;
            --road-color: #2c2c2c;
            --line-color: #fff;
            --card-bg: rgba(60, 60, 60, 0.95);
            --text-color: #FFFFFF;
            --accent-color: #00FF00;
            --danger-color: #FF0000;
            --warning-color: #FFA500;
            --font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        }

        body {
            font-family: var(--font-family);
            background-color: var(--bg-color);
            color: var(--text-color);
            height: 100vh;
            overflow: hidden;
            display: flex;
            justify-content: center;
            align-items: center;
            background-image: 
                linear-gradient(rgba(255, 255, 255, 0.05) 1px, transparent 1px),
                linear-gradient(90deg, rgba(255, 255, 255, 0.05) 1px, transparent 1px);
            background-size: 20px 20px;
        }

        .dashboard-container {
            width: 100vw;
            height: 100vh;
            max-width: 420px;
            max-height: 800px;
            display: flex;
            flex-direction: column;
            position: relative;
            background-color: var(--bg-color);
        }

        .road-container {
            flex: 1;
            position: relative;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 0;
        }

        .road {
            width: 45%;
            height: 100%;
            background-color: var(--road-color);
            position: relative;
            border-left: 3px solid var(--line-color);
            border-right: 3px solid var(--line-color);
            overflow: hidden;
        }
        
        .road-lines {
            position: absolute;
            width: 4px;
            height: 200%;
            background: linear-gradient(var(--line-color) 50%, transparent 50%);
            background-size: 100% 60px;
            left: 50%;
            transform: translateX(-50%);
            top: -100%;
            animation: moveRoad 8s linear infinite;
            animation-play-state: paused;
        }

        @keyframes moveRoad {
            from { top: -100%; }
            to { top: 0; }
        }

        .car-svg {
            position: absolute;
            width: 38%;
            height: auto;
            filter: drop-shadow(0 2px 8px rgba(0,0,0,0.6));
            z-index: 10;
        }

        .car-svg.small {
            width: 30%;
        }

        #main-car {
            bottom: 25%;
            left: 50%;
            transform: translateX(-50%);
        }

        #other-car {
            top: 25%;
            left: 50%;
            transform: translateX(-50%);
            opacity: 0;
            transition: opacity 0.3s ease;
        }
        
        #other-car.visible {
            opacity: 1;
        }

        .turn-indicator {
            position: absolute;
            width: 6px;
            height: 6px;
            background-color: var(--warning-color);
            border-radius: 50%;
            opacity: 0;
            box-shadow: 0 0 8px var(--warning-color);
            top: 30%;
        }

        .turn-indicator.left { left: 15%; }
        .turn-indicator.right { right: 15%; }

        .turn-indicator.blinking {
            animation: blink 0.8s infinite;
        }

        @keyframes blink {
            0%, 100% { opacity: 1; }
            50% { opacity: 0; }
        }

        .obstacle {
            position: absolute;
            width: 12%;
            left: 50%;
            transform: translateX(-50%);
            z-index: 8;
            display: none;
            transition: top 0.3s ease, bottom 0.3s ease;
        }

        .obstacle.front {
            top: calc(15% + (6 - var(--dist)) * 5%);
        }

        .obstacle.back {
            bottom: calc(15% + (6 - var(--dist)) * 5%);
        }
        
        .side-controls {
            position: absolute;
            top: 50%;
            transform: translateY(-50%);
            display: flex;
            flex-direction: column;
            gap: 12px;
            z-index: 20;
        }

        .side-controls.left { left: 10px; }
        .side-controls.right { right: 10px; }

        .control-card {
            background-color: var(--card-bg);
            border-radius: 10px;
            padding: 8px;
            width: 65px;
            height: 65px;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.15);
        }

        .switch-container {
            gap: 4px;
        }
        
        .switch-label {
            font-size: 8px;
            text-transform: uppercase;
            opacity: 0.9;
            font-weight: 600;
            text-align: center;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 38px;
            height: 20px;
        }

        .switch input { display: none; }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #444;
            border: 1px solid #666;
            transition: .3s;
            border-radius: 20px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 14px;
            width: 14px;
            left: 2px;
            bottom: 2px;
            background-color: #fff;
            transition: .3s;
            border-radius: 50%;
        }

        input:checked + .slider {
            background-color: var(--accent-color);
            border-color: var(--accent-color);
        }
        
        input:checked + .slider:before {
            transform: translateX(18px);
        }

        .compass, .speedometer {
            width: 100%;
            height: 100%;
            transition: transform 0.3s ease-out;
        }

        .compass-arrow {
            transform-origin: 50% 50%;
            transition: transform 0.3s ease-out;
        }

        .status-indicator {
            position: absolute;
            top: 12px;
            right: 12px;
            background-color: var(--card-bg);
            border-radius: 8px;
            padding: 6px 10px;
            display: none;
            align-items: center;
            gap: 6px;
            font-size: 10px;
            font-weight: 600;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.15);
        }

        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background-color: var(--accent-color);
            box-shadow: 0 0 6px var(--accent-color);
        }

        .hud {
            position: absolute;
            bottom: 0;
            left: 0;
            right: 0;
            padding: 12px;
            display: flex;
            flex-direction: column;
            gap: 8px;
            z-index: 20;
        }
        
        .hud-row {
            display: grid;
            grid-template-columns: 1fr 1fr 1fr;
            gap: 8px;
        }
        
        .hud-row.buttons {
            grid-template-columns: 1fr 1fr;
        }

        .hud-card {
            background-color: var(--card-bg);
            border-radius: 10px;
            padding: 10px;
            text-align: center;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.15);
        }

        .hud-card .value {
            font-size: 22px;
            font-weight: 600;
            line-height: 1;
        }
        
        .hud-card .unit {
            font-size: 12px;
            font-weight: 400;
            opacity: 0.8;
        }

        .hud-card .label {
            font-size: 9px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            opacity: 0.7;
            margin-top: 4px;
            font-weight: 600;
        }

        .hud-card.danger {
            background-color: rgba(255, 0, 0, 0.2);
            border-color: var(--danger-color);
            color: var(--danger-color);
        }

        .btn {
            background-color: var(--card-bg);
            border-radius: 10px;
            padding: 12px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            user-select: none;
            transition: all 0.2s ease;
            text-align: center;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.15);
        }

        .btn:active {
            transform: scale(0.95);
        }
        
        .btn.active {
            background-color: var(--accent-color);
            color: #000;
            border-color: var(--accent-color);
        }

        @media (max-height: 600px) {
            .control-card {
                width: 55px;
                height: 55px;
                padding: 6px;
            }
            
            .switch {
                width: 32px;
                height: 16px;
            }
            
            .switch-label {
                font-size: 7px;
            }
        }
    </style>
</head>
<body>
    <div class="dashboard-container">
        <div class="road-container">
            <div class="road">
                <div class="road-lines"></div>
            </div>

            <!-- Obstacle Icon (Cone) -->
            <svg class="obstacle" id="obstacle-icon" viewBox="0 0 100 100">
                <path d="M50 10 L20 85 L80 85 Z" fill="#FF6B00"/>
                <rect x="25" y="70" width="50" height="6" fill="#FFF"/>
                <rect x="30" y="50" width="40" height="6" fill="#FFF"/>
                <rect x="35" y="30" width="30" height="6" fill="#FFF"/>
            </svg>

            <!-- Main Car (Red) -->
            <svg id="main-car" class="car-svg" viewBox="0 0 100 180">
                <path d="M25 10 L75 10 C85 10 90 15 90 25 L90 155 C90 165 85 170 75 170 L25 170 C15 170 10 165 10 155 L10 25 C10 15 15 10 25 10 Z" fill="#DC143C"/>
                <rect x="20" y="35" width="60" height="40" fill="#1a1a1a" opacity="0.8"/>
                <rect x="15" y="30" width="70" height="50" fill="#FFB6C1" opacity="0.3"/>
                <div class="turn-indicator left"></div>
                <div class="turn-indicator right"></div>
            </svg>
            
            <!-- Other Car (Gray, Smaller) -->
            <svg id="other-car" class="car-svg small" viewBox="0 0 100 180">
                <path d="M25 10 L75 10 C85 10 90 15 90 25 L90 155 C90 165 85 170 75 170 L25 170 C15 170 10 165 10 155 L10 25 C10 15 15 10 25 10 Z" fill="#808080"/>
                <rect x="20" y="35" width="60" height="40" fill="#1a1a1a" opacity="0.8"/>
                <rect x="15" y="30" width="70" height="50" fill="#C0C0C0" opacity="0.3"/>
                <div class="turn-indicator left"></div>
                <div class="turn-indicator right"></div>
            </svg>
        </div>

        <!-- Left Side Controls -->
        <div class="side-controls left">
            <div class="control-card switch-container">
                <span class="switch-label">BUZZER</span>
                <label class="switch">
                    <input type="checkbox" id="buzzer-toggle" checked>
                    <span class="slider"></span>
                </label>
            </div>
            
            <div class="control-card switch-container">
                <span class="switch-label">AMBIENT</span>
                <label class="switch">
                    <input type="checkbox" id="ambient-toggle" checked>
                    <span class="slider"></span>
                </label>
            </div>
            
            <div class="control-card">
                <svg class="compass" viewBox="0 0 100 100">
                    <path d="M50 20 L60 50 L50 80 L40 50 Z" fill="#DC143C" class="compass-arrow" id="compass-arrow"/>
                    <text x="50" y="10" font-size="12" fill="#fff" text-anchor="middle" font-weight="600">N</text>
                    <text x="50" y="95" font-size="12" fill="#fff" text-anchor="middle" font-weight="600">S</text>
                    <text x="10" y="55" font-size="12" fill="#fff" text-anchor="middle" font-weight="600">W</text>
                    <text x="90" y="55" font-size="12" fill="#fff" text-anchor="middle" font-weight="600">E</text>
                </svg>
            </div>
        </div>

        <!-- Right Side Controls -->
        <div class="side-controls right">
            <div class="control-card">
                <svg class="speedometer" viewBox="0 0 100 100">
                    <path d="M20 80 A40 40 0 0 1 80 80" stroke="#666" stroke-width="4" fill="none"/>
                    <text id="speed-value" x="50" y="70" text-anchor="middle" font-size="24" font-weight="600" fill="#fff">0</text>
                    <text x="50" y="85" text-anchor="middle" font-size="10" fill="#fff" opacity="0.7">MPH</text>
                </svg>
            </div>
        </div>

        <!-- Status Indicator -->
        <div id="car2-status" class="status-indicator">
            <span>CAR 2</span>
            <div class="status-dot"></div>
        </div>

        <!-- Bottom HUD -->
        <div class="hud">
            <div class="hud-row">
                <div class="hud-card">
                    <div class="value" id="temp-value">--<span class="unit">°C</span></div>
                    <div class="label">TEMPERATURE</div>
                </div>
                <div class="hud-card">
                    <div class="value" id="humidity-value">--<span class="unit">%</span></div>
                    <div class="label">HUMIDITY</div>
                </div>
                <div class="hud-card" id="distance-card">
                    <div class="value" id="distance-value">--<span class="unit">CM</span></div>
                    <div class="label" id="distance-label">NO OBSTACLE</div>
                </div>
            </div>
            <div class="hud-row buttons">
                <div class="btn" id="turn-left-btn">← TURN LEFT</div>
                <div class="btn" id="turn-right-btn">TURN RIGHT →</div>
            </div>
        </div>
    </div>

    <script>
        const ws = new WebSocket(`ws://${window.location.hostname}/ws`);
        
        // UI Elements
        const elements = {
            temp: document.getElementById('temp-value'),
            humidity: document.getElementById('humidity-value'),
            distance: document.getElementById('distance-value'),
            distanceLabel: document.getElementById('distance-label'),
            distanceCard: document.getElementById('distance-card'),
            compass: document.getElementById('compass-arrow'),
            speed: document.getElementById('speed-value'),
            roadLines: document.querySelector('.road-lines'),
            mainCarLeft: document.querySelector('#main-car .turn-indicator.left'),
            mainCarRight: document.querySelector('#main-car .turn-indicator.right'),
            otherCarLeft: document.querySelector('#other-car .turn-indicator.left'),
            otherCarRight: document.querySelector('#other-car .turn-indicator.right'),
            turnLeftBtn: document.getElementById('turn-left-btn'),
            turnRightBtn: document.getElementById('turn-right-btn'),
            buzzerToggle: document.getElementById('buzzer-toggle'),
            ambientToggle: document.getElementById('ambient-toggle'),
            car2Status: document.getElementById('car2-status'),
            otherCar: document.getElementById('other-car'),
            obstacle: document.getElementById('obstacle-icon')
        };

        // WebSocket Events
        ws.onopen = () => console.log('Connected to Smart Car');
        ws.onclose = () => console.log('Disconnected from Smart Car');
        ws.onerror = (error) => console.error('WebSocket error:', error);

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            
            // Update sensor data
            elements.temp.innerHTML = `${data.temp.toFixed(1)}<span class="unit">°C</span>`;
            elements.humidity.innerHTML = `${data.humidity.toFixed(1)}<span class="unit">%</span>`;

            // Handle obstacle detection
            updateObstacle(data.frontDist, data.backDist);

            // Update compass
            elements.compass.style.transform = `rotate(${data.direction}deg)`;
            
            // Update speed
            elements.speed.textContent = data.speed;
            elements.roadLines.style.animationPlayState = data.speed > 0 ? 'running' : 'paused';

            // Update turn indicators
            elements.mainCarLeft.classList.toggle('blinking', data.leftIndicator);
            elements.mainCarRight.classList.toggle('blinking', data.rightIndicator);
            elements.turnLeftBtn.classList.toggle('active', data.leftIndicator);
            elements.turnRightBtn.classList.toggle('active', data.rightIndicator);

            // Update Car 2 status
            elements.car2Status.style.display = data.car2Connected ? 'flex' : 'none';
            elements.otherCar.classList.toggle('visible', data.car2Connected);
            
            if (data.car2Connected) {
                elements.otherCarLeft.classList.toggle('blinking', data.car2Left);
                elements.otherCarRight.classList.toggle('blinking', data.car2Right);
            }

            // Update toggles
            elements.buzzerToggle.checked = data.buzzerOn;
            elements.ambientToggle.checked = data.ambientOn;
        };

        function updateObstacle(frontDist, backDist) {
            let showObstacle = false;
            let distance = 0;
            let position = 'front';
            let label = 'NO OBSTACLE';

            if (frontDist < 6) {
                showObstacle = true;
                distance = frontDist;
                position = 'front';
                label = 'DANGER - FRONT';
            } else if (backDist < 6) {
                showObstacle = true;
                distance = backDist;
                position = 'back';
                label = 'DANGER - BACK';
            }

            if (showObstacle) {
                elements.obstacle.style.display = 'block';
                elements.obstacle.className = `obstacle ${position}`;
                elements.obstacle.style.setProperty('--dist', distance);
                elements.distance.innerHTML = `${distance.toFixed(0)}<span class="unit">CM</span>`;
                elements.distanceLabel.textContent = label;
                elements.distanceCard.classList.add('danger');
            } else {
                elements.obstacle.style.display = 'none';
                elements.distance.innerHTML = '--<span class="unit">CM</span>';
                elements.distanceLabel.textContent = 'NO OBSTACLE';
                elements.distanceCard.classList.remove('danger');
            }
        }

        function sendMessage(message) {
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify(message));
            }
        }

        // Event Listeners
        elements.turnLeftBtn.addEventListener('click', () => {
            sendMessage({ action: 'left_indicator' });
        });

        elements.turnRightBtn.addEventListener('click', () => {
            sendMessage({ action: 'right_indicator' });
        });

        elements.buzzerToggle.addEventListener('change', (e) => {
            sendMessage({ action: 'buzzer_toggle', value: e.target.checked });
        });

        elements.ambientToggle.addEventListener('change', (e) => {
            sendMessage({ action: 'ambient_toggle', value: e.target.checked });
        });

        // Disable context menu and text selection
        document.addEventListener('contextmenu', e => e.preventDefault());
        document.addEventListener('selectstart', e => e.preventDefault());
        document.addEventListener('dragstart', e => e.preventDefault());
    </script>
</body>
</html>
)rawliteral";
