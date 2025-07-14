#ifndef WEB_H
#define WEB_H

String getWebPage() {
  return R"rawliteral(
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
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #0f0f0f 0%, #1a1a1a 100%);
            color: #fff;
            overflow: hidden;
            user-select: none;
            height: 100vh;
        }
        
        .container {
            height: 100vh;
            display: flex;
            flex-direction: column;
        }
        
        .map-area {
            flex: 1;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            position: relative;
            overflow: hidden;
        }
        
        .road {
            position: absolute;
            top: 50%;
            left: 20%;
            right: 20%;
            height: 60px;
            background: #2a2a2a;
            border-radius: 8px;
            transform: translateY(-50%);
            display: flex;
            align-items: center;
            justify-content: center;
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.3);
        }
        
        .road::before {
            content: '';
            position: absolute;
            top: 50%;
            left: 5%;
            right: 5%;
            height: 2px;
            background: repeating-linear-gradient(
                90deg,
                #ffff00 0px,
                #ffff00 15px,
                transparent 15px,
                transparent 30px
            );
            transform: translateY(-50%);
        }
        
        .car {
            width: 35px;
            height: 18px;
            background: #007bff;
            border-radius: 8px;
            position: relative;
            box-shadow: 0 2px 8px rgba(0,0,0,0.3);
        }
        
        .car::before,
        .car::after {
            content: '';
            position: absolute;
            width: 6px;
            height: 16px;
            background: #ff8c00;
            top: 1px;
            opacity: 0;
            transition: opacity 0.1s;
        }
        
        .car::before {
            left: -10px;
            transform: rotate(90deg);
        }
        
        .car::after {
            right: -10px;
            transform: rotate(90deg);
        }
        
        .car.left-indicator::before {
            opacity: 1;
            animation: blink 1s infinite;
        }
        
        .car.right-indicator::after {
            opacity: 1;
            animation: blink 1s infinite;
        }
        
        @keyframes blink {
            0%, 50% { opacity: 1; }
            51%, 100% { opacity: 0; }
        }
        
        .hud {
            position: absolute;
            top: 15px;
            left: 15px;
            right: 15px;
            display: flex;
            justify-content: space-between;
            align-items: flex-start;
        }
        
        .speed-display {
            background: rgba(0, 0, 0, 0.8);
            padding: 12px 18px;
            border-radius: 12px;
            font-size: 20px;
            font-weight: bold;
            color: #00ff00;
            border: 2px solid #007bff;
            backdrop-filter: blur(5px);
        }
        
        .compass {
            width: 70px;
            height: 70px;
            background: rgba(0, 0, 0, 0.8);
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            border: 2px solid #007bff;
            position: relative;
            backdrop-filter: blur(5px);
        }
        
        .compass-arrow {
            width: 0;
            height: 0;
            border-left: 6px solid transparent;
            border-right: 6px solid transparent;
            border-bottom: 25px solid #ff0000;
            transform-origin: 50% 80%;
            transition: transform 0.3s ease;
        }
        
        .compass::before {
            content: 'N';
            position: absolute;
            top: 8px;
            font-size: 11px;
            color: #fff;
            font-weight: bold;
        }
        
        .alert {
            position: absolute;
            top: 100px;
            left: 15px;
            right: 15px;
            padding: 12px;
            background: rgba(220, 53, 69, 0.95);
            color: white;
            border-radius: 8px;
            text-align: center;
            transform: translateY(-150px);
            opacity: 0;
            transition: all 0.3s ease;
            font-weight: bold;
            backdrop-filter: blur(5px);
        }
        
        .alert.show {
            transform: translateY(0);
            opacity: 1;
        }
        
        .controls {
            height: 200px;
            background: rgba(15, 15, 15, 0.95);
            backdrop-filter: blur(10px);
            padding: 15px;
            border-top: 1px solid #333;
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
            margin-bottom: 15px;
        }
        
        .status-card {
            background: rgba(255, 255, 255, 0.1);
            padding: 12px;
            border-radius: 8px;
            text-align: center;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        
        .status-value {
            font-size: 16px;
            font-weight: bold;
            color: #007bff;
            margin-bottom: 4px;
        }
        
        .status-label {
            font-size: 11px;
            color: #aaa;
        }
        
        .button-row {
            display: flex;
            gap: 12px;
        }
        
        .control-btn {
            flex: 1;
            padding: 18px;
            background: #333;
            color: white;
            border: 2px solid #555;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.2s ease;
            text-align: center;
        }
        
        .control-btn:active {
            transform: scale(0.95);
        }
        
        .control-btn.left {
            background: #28a745;
            border-color: #28a745;
        }
        
        .control-btn.right {
            background: #ffc107;
            color: #000;
            border-color: #ffc107;
        }
        
        .control-btn.active {
            background: #ff6b00 !important;
            border-color: #ff6b00 !important;
            color: #fff !important;
            box-shadow: 0 0 15px rgba(255, 107, 0, 0.5);
        }
        
        @media (max-width: 480px) {
            .status-grid {
                grid-template-columns: repeat(2, 1fr);
            }
            
            .hud {
                flex-direction: column;
                align-items: flex-start;
                gap: 8px;
            }
            
            .speed-display {
                font-size: 18px;
                padding: 10px 15px;
            }
            
            .compass {
                width: 60px;
                height: 60px;
            }
            
            .control-btn {
                padding: 15px;
                font-size: 14px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="map-area">
            <div class="road">
                <div class="car" id="car"></div>
            </div>
            
            <div class="hud">
                <div class="speed-display">
                    <div id="speedDisplay">0 km/h</div>
                </div>
                <div class="compass">
                    <div class="compass-arrow" id="compassArrow"></div>
                </div>
            </div>
            
            <div class="alert" id="alertBox">
                <div id="alertText">Obstacle Detected!</div>
            </div>
        </div>
        
        <div class="controls">
            <div class="status-grid">
                <div class="status-card">
                    <div class="status-value" id="temperature">20°C</div>
                    <div class="status-label">Temperature</div>
                </div>
                <div class="status-card">
                    <div class="status-value" id="humidity">50%</div>
                    <div class="status-label">Humidity</div>
                </div>
                <div class="status-card">
                    <div class="status-value" id="proximity">Clear</div>
                    <div class="status-label">Proximity</div>
                </div>
            </div>
            
            <div class="button-row">
                <button class="control-btn left" id="leftBtn" onclick="toggleLeft()">← LEFT</button>
                <button class="control-btn right" id="rightBtn" onclick="toggleRight()">RIGHT →</button>
            </div>
        </div>
    </div>
    
    <script>
        let isUpdating = false;
        
        function toggleLeft() {
            if (isUpdating) return;
            fetch('/api/indicator/left', { method: 'POST' })
                .catch(err => console.error('Left toggle error:', err));
        }
        
        function toggleRight() {
            if (isUpdating) return;
            fetch('/api/indicator/right', { method: 'POST' })
                .catch(err => console.error('Right toggle error:', err));
        }
        
        function updateDashboard() {
            if (isUpdating) return;
            isUpdating = true;
            
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    // Update status displays
                    document.getElementById('temperature').textContent = data.temperature.toFixed(1) + '°C';
                    document.getElementById('humidity').textContent = data.humidity.toFixed(1) + '%';
                    document.getElementById('speedDisplay').textContent = data.speed.toFixed(1) + ' km/h';
                    
                    // Update proximity with distance
                    let proximityText = data.obstacleLocation;
                    if (data.obstacleAlert && data.minDistance > 0) {
                        proximityText += ` (${data.minDistance}cm)`;
                    }
                    document.getElementById('proximity').textContent = proximityText;
                    
                    // Update compass
                    document.getElementById('compassArrow').style.transform = `rotate(${data.direction}deg)`;
                    
                    // Update car indicators
                    const car = document.getElementById('car');
                    car.className = 'car';
                    if (data.leftIndicator) car.classList.add('left-indicator');
                    if (data.rightIndicator) car.classList.add('right-indicator');
                    
                    // Update buttons
                    document.getElementById('leftBtn').classList.toggle('active', data.leftIndicator);
                    document.getElementById('rightBtn').classList.toggle('active', data.rightIndicator);
                    
                    // Update alert
                    const alertBox = document.getElementById('alertBox');
                    const alertText = document.getElementById('alertText');
                    if (data.obstacleAlert) {
                        alertText.textContent = `Obstacle: ${data.obstacleLocation}${data.minDistance > 0 ? ' (' + data.minDistance + 'cm)' : ''}`;
                        alertBox.classList.add('show');
                    } else {
                        alertBox.classList.remove('show');
                    }
                    
                    isUpdating = false;
                })
                .catch(error => {
                    console.error('Update error:', error);
                    isUpdating = false;
                });
        }
        
        // Update every 200ms
        setInterval(updateDashboard, 200);
        updateDashboard();
    </script>
</body>
</html>
)rawliteral";
}

#endif
