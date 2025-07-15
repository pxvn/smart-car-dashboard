#pragma once

#include <pgmspace.h>

const char WEB_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Smart Car Dash</title>
    <style>
        :root {
            --bg-color: #3a3a3a; /* Darker grey for the background grid */
            --road-color: #424242;
            --line-color: #BDBDBD;
            --card-bg: rgba(96, 96, 96, 0.9); /* Semi-transparent grey */
            --text-color: #FFFFFF;
            --accent-color: #4CAF50;
            --font-family: 'Roboto', -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
        }

        body {
            margin: 0;
            font-family: var(--font-family);
            background-color: var(--bg-color);
            color: var(--text-color);
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            overflow: hidden;
            -webkit-tap-highlight-color: transparent;
        }

        .dashboard-container {
            width: 100%;
            height: 100%;
            max-width: 450px;
            max-height: 950px;
            background-color: var(--bg-color);
            display: flex;
            flex-direction: column;
            position: relative;
            overflow: hidden;
            background: 
                linear-gradient(rgba(255, 255, 255, 0.07) 1px, transparent 1px),
                linear-gradient(90deg, rgba(255, 255, 255, 0.07) 1px, transparent 1px);
            background-size: 25px 25px;
        }

        .road-container {
            flex-grow: 1;
            position: relative;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .road {
            width: 50%;
            height: 100%;
            background-color: var(--road-color);
            position: relative;
            border-left: 2px solid var(--line-color);
            border-right: 2px solid var(--line-color);
            overflow: hidden;
        }
        
        .road-lines {
            position: absolute;
            width: 6px;
            height: 200%;
            background: linear-gradient(var(--line-color) 60%, transparent 60%);
            background-size: 100% 80px;
            left: 50%;
            transform: translateX(-50%);
            top: -100%;
            animation: moveRoad 10s linear infinite;
            animation-play-state: paused;
        }

        @keyframes moveRoad {
            from { top: -100%; }
            to { top: 0; }
        }

        .car-svg {
            position: absolute;
            width: 45%;
            max-width: 100px;
            height: auto;
            filter: drop-shadow(0 4px 8px rgba(0,0,0,0.4));
        }

        #car1 {
            bottom: 25%; /* Adjusted position */
            left: 50%;
            transform: translateX(-50%);
            z-index: 10;
        }

        #car2 {
            top: 20%; /* Adjusted position */
            left: 50%;
            transform: translateX(-50%);
            z-index: 5;
            opacity: 0;
            transition: opacity 0.5s;
        }
        
        #car2.visible {
            opacity: 1;
        }

        .indicator {
            position: absolute;
            width: 8px;
            height: 8px;
            background-color: #FFC107;
            border-radius: 50%;
            opacity: 0;
            box-shadow: 0 0 5px #FFC107, 0 0 10px #FFC107;
        }

        .blinking {
            animation: blink 1s infinite;
        }

        @keyframes blink {
            0%, 100% { opacity: 1; }
            50% { opacity: 0; }
        }

        #car1-left-indicator { top: 28%; left: 12%; }
        #car1-right-indicator { top: 28%; right: 12%; }
        #car2-left-indicator { top: 28%; left: 12%; }
        #car2-right-indicator { top: 28%; right: 12%; }

        #obstacle-cone {
            position: absolute;
            width: 10%;
            max-width: 30px;
            top: 35%;
            left: 50%;
            transform: translateX(-50%);
            z-index: 8;
            display: none;
        }
        
        .side-controls-left {
            position: absolute;
            left: 15px;
            top: 50%;
            transform: translateY(-50%);
            display: flex;
            flex-direction: column;
            gap: 15px;
            z-index: 20;
        }

        .control-card {
            background-color: var(--card-bg);
            border-radius: 12px;
            padding: 10px;
            width: 60px;
            height: 60px;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            backdrop-filter: blur(5px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .switch-container {
            gap: 5px;
        }
        
        .switch-label {
            font-size: 9px;
            text-transform: uppercase;
            opacity: 0.8;
            font-weight: 500;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 44px; /* Slightly wider */
            height: 26px;
        }

        .switch input { display: none; }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #333;
            border: 1px solid #555;
            transition: .4s;
            border-radius: 26px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 20px;
            width: 20px;
            left: 2px;
            bottom: 2px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }

        input:checked + .slider {
            background-color: #222;
        }
        
        input:checked + .slider:before {
            background-color: var(--accent-color);
            transform: translateX(18px);
        }
        
        .side-controls-right {
            position: absolute;
            right: 15px;
            top: 50%;
            transform: translateY(-50%);
            z-index: 20;
        }
        
        #compass-rose, #speedo-gauge {
            width: 100%;
            height: 100%;
            transition: transform 0.5s ease-out;
        }

        .hud {
            position: absolute;
            bottom: 0;
            left: 0;
            width: 100%;
            padding: 15px;
            box-sizing: border-box;
            display: flex;
            flex-direction: column;
            gap: 10px;
            z-index: 20;
        }
        
        .hud-row {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
        }
        
        .hud-row.buttons {
            grid-template-columns: 1fr 1fr;
        }

        .hud-card, .btn {
            background-color: var(--card-bg);
            border-radius: 12px;
            padding: 12px;
            text-align: center;
            backdrop-filter: blur(5px);
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        .hud-card .value {
            font-size: 28px;
            font-weight: 500;
        }
        
        .hud-card .value .unit {
            font-size: 16px;
            font-weight: 300;
        }

        .hud-card .label {
            font-size: 10px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            opacity: 0.7;
            margin-top: 4px;
        }

        .btn {
            padding: 15px;
            font-size: 16px;
            font-weight: 500;
            cursor: pointer;
            user-select: none;
            transition: background-color 0.2s;
        }

        .btn:active {
            background-color: rgba(120, 120, 120, 0.9);
        }
        
        .btn.active {
            background-color: var(--accent-color);
            color: white;
        }
        
        .status-card {
            position: absolute;
            top: 15px;
            right: 15px;
            background-color: var(--card-bg);
            border-radius: 12px;
            padding: 8px 12px;
            display: flex;
            align-items: center;
            gap: 8px;
            z-index: 20;
            border: 1px solid rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(5px);
            display: none;
        }
        
        .status-card .label {
            font-size: 12px;
            font-weight: 500;
        }
        
        .status-indicator {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background-color: var(--accent-color);
            box-shadow: 0 0 8px var(--accent-color);
        }

    </style>
</head>
<body>
    <div class="dashboard-container">
        <!-- Road and Cars -->
        <div class="road-container">
            <div class="road">
                <div class="road-lines"></div>
            </div>

            <svg id="obstacle-cone" viewBox="0 0 80 80" xmlns="http://www.w3.org/2000/svg"><g><path fill="#FF9800" d="M40 5 L10 75 L70 75 Z"/><path fill="#FFFFFF" d="M15 60 H65 V68 H15 Z"/><path fill="#FF9800" d="M20 40 H60 V48 H20 Z"/><path fill="#FFFFFF" d="M25 20 H55 V28 H25 Z"/></g></svg>

            <div id="car2" class="car-svg"><svg viewBox="0 0 200 400"><path d="M 60,10 L 140,10 C 180,10 190,30 190,60 L 190,350 C 190,380 180,390 140,390 L 60,390 C 20,390 10,380 10,350 L 10,60 C 10,30 20,10 60,10 Z" fill="#E0E0E0"/><path d="M 40,80 L 160,80 L 160,200 L 40,200 Z" fill="#212121" opacity="0.8"/><path d="M 30,70 L 170,70 L 160,210 L 40,210 Z" fill="#424242" opacity="0.5"/></svg><div id="car2-left-indicator" class="indicator"></div><div id="car2-right-indicator" class="indicator"></div></div>
            
            <div id="car1" class="car-svg"><svg viewBox="0 0 200 400"><path d="M 60,10 L 140,10 C 180,10 190,30 190,60 L 190,350 C 190,380 180,390 140,390 L 60,390 C 20,390 10,380 10,350 L 10,60 C 10,30 20,10 60,10 Z" fill="#D32F2F"/><path d="M 40,80 L 160,80 L 160,200 L 40,200 Z" fill="#212121" opacity="0.8"/><path d="M 30,70 L 170,70 L 160,210 L 40,210 Z" fill="#E57373" opacity="0.5"/></svg><div id="car1-left-indicator" class="indicator"></div><div id="car1-right-indicator" class="indicator"></div></div>
        </div>

        <!-- Left Controls -->
        <div class="side-controls-left">
            <div class="control-card switch-container">
                <span class="switch-label">Buzzer</span>
                <label class="switch">
                    <input type="checkbox" id="buzzer-toggle" checked>
                    <span class="slider"></span>
                </label>
            </div>
            <div class="control-card switch-container">
                <span class="switch-label">Ambient</span>
                <label class="switch">
                    <input type="checkbox" id="ambient-toggle" checked>
                    <span class="slider"></span>
                </label>
            </div>
            <div class="control-card" id="compass-container">
                 <svg id="compass-rose" viewBox="0 0 100 100"><circle cx="50" cy="50" r="48" fill="none" stroke="#E0E0E0" stroke-width="2" opacity="0.5"/><path d="M50 10 L45 20 L55 20 Z" fill="#D32F2F"/><path d="M50 90 L45 80 L55 80 Z" fill="#E0E0E0"/><text x="50" y="8" font-size="12" fill="#E0E0E0" text-anchor="middle">N</text><text x="50" y="98" font-size="12" fill="#E0E0E0" text-anchor="middle">S</text><text x="8" y="54" font-size="12" fill="#E0E0E0" text-anchor="middle">W</text><text x="92" y="54" font-size="12" fill="#E0E0E0" text-anchor="middle">E</text></svg>
            </div>
        </div>

        <!-- Right Controls -->
        <div class="side-controls-right">
            <div class="control-card" id="speedo-container">
                <svg id="speedo-gauge" viewBox="0 0 100 100">
                    <path d="M 20 80 A 40 40 0 0 1 80 80" stroke="#E0E0E0" stroke-width="4" fill="none" opacity="0.5" stroke-linecap="round"/>
                    <text id="speed-text" x="50" y="70" text-anchor="middle" font-size="24" font-weight="500" fill="#E0E0E0">0</text>
                    <text x="50" y="85" text-anchor="middle" font-size="10" fill="#E0E0E0" opacity="0.7">MPH</text>
                </svg>
            </div>
        </div>

        <!-- Status Card -->
        <div id="car2-status" class="status-card">
            <span class="label">CAR 2 CONNECTED</span>
            <div class="status-indicator"></div>
        </div>

        <!-- Bottom HUD -->
        <div class="hud">
            <div class="hud-row">
                <div class="hud-card" id="temp-card">
                    <div class="value" id="temp-value">--<span class="unit">&deg;C</span></div>
                    <div class="label">Temperature</div>
                </div>
                <div class="hud-card" id="humidity-card">
                    <div class="value" id="humidity-value">--<span class="unit">%</span></div>
                    <div class="label">Humidity</div>
                </div>
                <div class="hud-card" id="obstacle-card">
                    <div class="value" id="obstacle-value">--<span class="unit">cm</span></div>
                    <div class="label" id="obstacle-label">NO OBSTACLE</div>
                </div>
            </div>
            <div class="hud-row buttons">
                <div class="btn" id="turn-left-btn">TURN LEFT</div>
                <div class="btn" id="turn-right-btn">TURN RIGHT</div>
            </div>
        </div>
    </div>

    <script>
        // The JavaScript remains the same as it targets the same element IDs.
        const ws = new WebSocket(`ws://${window.location.hostname}/ws`);

        const tempValue = document.getElementById('temp-value');
        const humidityValue = document.getElementById('humidity-value');
        const obstacleValue = document.getElementById('obstacle-value');
        const obstacleLabel = document.getElementById('obstacle-label');
        const compass = document.getElementById('compass-rose');
        const speedText = document.getElementById('speed-text');
        const roadLines = document.querySelector('.road-lines');
        
        const car1LeftIndicator = document.getElementById('car1-left-indicator');
        const car1RightIndicator = document.getElementById('car1-right-indicator');
        const car2LeftIndicator = document.getElementById('car2-left-indicator');
        const car2RightIndicator = document.getElementById('car2-right-indicator');
        
        const turnLeftBtn = document.getElementById('turn-left-btn');
        const turnRightBtn = document.getElementById('turn-right-btn');
        
        const buzzerToggle = document.getElementById('buzzer-toggle');
        const ambientToggle = document.getElementById('ambient-toggle');

        const car2Status = document.getElementById('car2-status');
        const car2Element = document.getElementById('car2');
        const obstacleCone = document.getElementById('obstacle-cone');

        ws.onopen = () => console.log('WebSocket connected');
        ws.onclose = () => console.log('WebSocket disconnected');
        ws.onerror = (error) => console.log('WebSocket error:', error);

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            
            tempValue.innerHTML = `${data.temp.toFixed(1)}<span class="unit">&deg;C</span>`;
            humidityValue.innerHTML = `${data.humidity.toFixed(1)}<span class="unit">%</span>`;

            if (data.frontDist < 100) {
                obstacleCone.style.display = 'block';
                obstacleValue.innerHTML = `${data.frontDist.toFixed(0)}<span class="unit">cm</span>`;
                obstacleLabel.textContent = 'OBSTACLE AT FRONT';
                let coneTop = 40 - (data.frontDist / 1.5);
                obstacleCone.style.top = `${Math.max(20, coneTop)}%`;
            } else if (data.backDist < 100) {
                 obstacleCone.style.display = 'none';
                 obstacleValue.innerHTML = `${data.backDist.toFixed(0)}<span class="unit">cm</span>`;
                 obstacleLabel.textContent = 'OBSTACLE AT BACK';
            } else {
                obstacleCone.style.display = 'none';
                obstacleValue.innerHTML = `--<span class="unit">cm</span>`;
                obstacleLabel.textContent = 'NO OBSTACLE';
            }

            compass.style.transform = `rotate(${data.direction}deg)`;
            speedText.textContent = data.speed;
            roadLines.style.animationPlayState = data.speed > 0 ? 'running' : 'paused';

            car1LeftIndicator.classList.toggle('blinking', data.leftIndicator);
            car1RightIndicator.classList.toggle('blinking', data.rightIndicator);
            turnLeftBtn.classList.toggle('active', data.leftIndicator);
            turnRightBtn.classList.toggle('active', data.rightIndicator);

            car2Status.style.display = data.car2Connected ? 'flex' : 'none';
            car2Element.classList.toggle('visible', data.car2Connected);
            if(data.car2Connected) {
                car2LeftIndicator.classList.toggle('blinking', data.car2Left);
                car2RightIndicator.classList.toggle('blinking', data.car2Right);
            } else {
                car2LeftIndicator.classList.remove('blinking');
                car2RightIndicator.classList.remove('blinking');
            }

            buzzerToggle.checked = data.buzzerOn;
            ambientToggle.checked = data.ambientOn;
        };

        function sendWsMessage(message) {
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify(message));
            }
        }

        turnLeftBtn.addEventListener('click', () => sendWsMessage({ action: 'left_indicator' }));
        turnRightBtn.addEventListener('click', () => sendWsMessage({ action: 'right_indicator' }));
        buzzerToggle.addEventListener('change', (e) => sendWsMessage({ action: 'buzzer_toggle', value: e.target.checked }));
        ambientToggle.addEventListener('change', (e) => sendWsMessage({ action: 'ambient_toggle', value: e.target.checked }));

    </script>
</body>
</html>
)rawliteral";
