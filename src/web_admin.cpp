#include "web_admin.h"
#include "sensors.h"
#include "imu.h"
#include "motors.h"
#include "evasion.h"
#include "comm_pi.h"
#include <ArduinoJson.h>

WebAdminManager webAdmin;

// Pure Remote Controller Web Interface (Zero monitoring clutter, pure control & E-Stop)
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>TinkerHub Bot | Remote Controller</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@400;600;700;900&family=JetBrains+Mono:wght@600&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-base: #080B11;
      --card-bg: rgba(17, 24, 39, 0.85);
      --border-color: rgba(255, 255, 255, 0.1);
      --cyan-neon: #00F0FF;
      --cyan-glow: rgba(0, 240, 255, 0.25);
      --red-neon: #EF4444;
      --red-glow: rgba(239, 68, 68, 0.4);
      --emerald-neon: #10B981;
      --text-primary: #F8FAFC;
      --text-secondary: #94A3B8;
    }

    * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-user-select: none; }
    body {
      background: radial-gradient(circle at 50% 0%, #151D2F 0%, var(--bg-base) 80%);
      color: var(--text-primary);
      font-family: 'Outfit', sans-serif;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 16px;
    }

    .container {
      width: 100%;
      max-width: 440px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 6px 4px;
    }

    .brand {
      display: flex;
      align-items: center;
      gap: 10px;
    }

    .brand-icon {
      width: 36px;
      height: 36px;
      border-radius: 8px;
      background: linear-gradient(135deg, #00F0FF, #3B82F6);
      display: flex;
      align-items: center;
      justify-content: center;
      font-weight: 900;
      color: #000;
      font-size: 16px;
      box-shadow: 0 0 15px var(--cyan-glow);
    }

    .brand-text h1 { font-size: 17px; font-weight: 700; letter-spacing: 0.5px; }
    .brand-text p { font-size: 11px; color: var(--cyan-neon); font-family: 'JetBrains Mono', monospace; }

    .badge {
      padding: 6px 12px;
      border-radius: 20px;
      font-size: 12px;
      font-weight: 700;
      background: rgba(16, 185, 129, 0.15);
      border: 1px solid var(--emerald-neon);
      color: var(--emerald-neon);
    }
    .badge.estop-alert {
      background: rgba(239, 68, 68, 0.2);
      border-color: var(--red-neon);
      color: var(--red-neon);
      animation: pulseAlert 1s infinite alternate;
    }

    @keyframes pulseAlert {
      from { box-shadow: 0 0 5px var(--red-neon); }
      to { box-shadow: 0 0 18px var(--red-neon); }
    }

    /* EMERGENCY STOP BUTTON */
    .estop-box {
      width: 100%;
    }

    .btn-estop {
      width: 100%;
      padding: 20px;
      border-radius: 16px;
      background: linear-gradient(180deg, #DC2626 0%, #991B1B 100%);
      border: 2px solid #EF4444;
      color: #FFF;
      font-size: 20px;
      font-weight: 900;
      letter-spacing: 2px;
      text-transform: uppercase;
      cursor: pointer;
      box-shadow: 0 6px 25px var(--red-glow);
      transition: all 0.15s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 12px;
    }
    .btn-estop:active {
      transform: scale(0.97);
      background: #7F1D1D;
      box-shadow: 0 2px 10px var(--red-glow);
    }

    .btn-resume {
      width: 100%;
      padding: 16px;
      border-radius: 16px;
      background: linear-gradient(180deg, #059669 0%, #047857 100%);
      border: 2px solid #10B981;
      color: #FFF;
      font-size: 17px;
      font-weight: 800;
      letter-spacing: 1.5px;
      text-transform: uppercase;
      cursor: pointer;
      box-shadow: 0 4px 20px rgba(16, 185, 129, 0.35);
      display: none;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }
    .btn-resume:active { transform: scale(0.97); background: #065F46; }

    /* CARD CONTAINER */
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      border: 1px solid var(--border-color);
      border-radius: 20px;
      padding: 20px;
      box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5);
    }

    .card-title {
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 1.5px;
      color: var(--text-secondary);
      margin-bottom: 12px;
      font-weight: 700;
    }

    /* MODE SELECTOR */
    .mode-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 8px;
    }

    .btn-mode {
      padding: 12px 6px;
      border-radius: 12px;
      background: rgba(255, 255, 255, 0.04);
      border: 1px solid var(--border-color);
      color: var(--text-secondary);
      font-size: 12px;
      font-weight: 700;
      cursor: pointer;
      transition: all 0.2s ease;
      text-align: center;
    }

    .btn-mode.active {
      background: linear-gradient(135deg, rgba(0, 240, 255, 0.2), rgba(59, 130, 246, 0.2));
      border-color: var(--cyan-neon);
      color: var(--cyan-neon);
      box-shadow: 0 0 15px var(--cyan-glow);
    }

    /* TANK CONTROLLER */
    .dpad-container {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 12px;
      margin: 16px auto;
      max-width: 320px;
    }

    .ctrl-btn {
      aspect-ratio: 1;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.06);
      border: 1px solid var(--border-color);
      color: var(--text-primary);
      font-size: 26px;
      font-weight: bold;
      display: flex;
      align-items: center;
      justify-content: center;
      cursor: pointer;
      transition: all 0.1s ease;
      box-shadow: 0 4px 14px rgba(0, 0, 0, 0.35);
      touch-action: none;
    }

    .ctrl-btn:active, .ctrl-btn.active {
      transform: scale(0.92);
      background: var(--cyan-neon);
      color: #000;
      box-shadow: 0 0 25px var(--cyan-neon);
    }

    .ctrl-btn.stop-btn {
      background: rgba(239, 68, 68, 0.15);
      border-color: rgba(239, 68, 68, 0.4);
      color: var(--red-neon);
      font-size: 14px;
      font-weight: 900;
    }
    .ctrl-btn.stop-btn:active { background: var(--red-neon); color: #FFF; }

    /* SLIDERS */
    .slider-group {
      margin-top: 14px;
    }

    .slider-label {
      display: flex;
      justify-content: space-between;
      font-size: 13px;
      color: var(--text-secondary);
      margin-bottom: 6px;
      font-weight: 600;
    }
    .slider-label span { color: var(--cyan-neon); font-family: 'JetBrains Mono', monospace; }

    input[type=range] {
      width: 100%;
      height: 6px;
      border-radius: 4px;
      background: rgba(255, 255, 255, 0.1);
      outline: none;
      -webkit-appearance: none;
    }

    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--cyan-neon);
      cursor: pointer;
      box-shadow: 0 0 10px var(--cyan-neon);
    }

    .disabled-overlay {
      pointer-events: none;
      opacity: 0.35;
      filter: grayscale(0.8);
    }

    /* ULTRASONIC RADAR COMPASS */
    .sensor-compass {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 8px;
      margin: 4px 0 2px 0;
    }

    .sensor-mid-row {
      display: flex;
      align-items: center;
      justify-content: space-between;
      width: 100%;
      gap: 8px;
    }

    .sensor-node {
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid var(--border-color);
      border-radius: 14px;
      padding: 8px 12px;
      display: flex;
      flex-direction: column;
      align-items: center;
      min-width: 104px;
      transition: all 0.2s ease;
    }

    .sensor-node.warning {
      border-color: #F59E0B;
      background: rgba(245, 158, 11, 0.12);
      box-shadow: 0 0 12px rgba(245, 158, 11, 0.3);
    }

    .sensor-node.danger {
      border-color: var(--red-neon);
      background: rgba(239, 68, 68, 0.2);
      box-shadow: 0 0 16px var(--red-glow);
      animation: pulseAlert 0.8s infinite alternate;
    }

    .sensor-dir {
      font-size: 10px;
      font-weight: 700;
      letter-spacing: 1px;
      color: var(--text-secondary);
      text-transform: uppercase;
    }

    .sensor-val {
      font-size: 15px;
      font-weight: 800;
      font-family: 'JetBrains Mono', monospace;
      color: var(--cyan-neon);
      margin: 2px 0;
    }

    .sensor-node.danger .sensor-val { color: var(--red-neon); }
    .sensor-node.warning .sensor-val { color: #F59E0B; }

    .sensor-meter {
      width: 100%;
      height: 4px;
      background: rgba(255, 255, 255, 0.08);
      border-radius: 2px;
      overflow: hidden;
      margin-top: 2px;
    }

    .meter-bar {
      height: 100%;
      width: 100%;
      background: var(--cyan-neon);
      border-radius: 2px;
      transition: width 0.15s ease, background 0.15s ease;
    }

    .robot-center-icon {
      position: relative;
      width: 44px;
      height: 44px;
      border-radius: 50%;
      background: rgba(0, 240, 255, 0.08);
      border: 1px solid rgba(0, 240, 255, 0.3);
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 20px;
      flex-shrink: 0;
    }

    .robot-pulse {
      position: absolute;
      width: 100%;
      height: 100%;
      border-radius: 50%;
      border: 1px solid var(--cyan-neon);
      opacity: 0.4;
      animation: ping 2s cubic-bezier(0, 0, 0.2, 1) infinite;
    }

    @keyframes ping {
      75%, 100% {
        transform: scale(1.6);
        opacity: 0;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <div class="brand">
        <div class="brand-icon">TB</div>
        <div class="brand-text">
          <h1>TINKERHUB BOT</h1>
          <p>REMOTE CONTROLLER</p>
        </div>
      </div>
      <div class="badge estop-alert" id="systemBadge">E-STOPPED</div>
    </header>

    <!-- EMERGENCY STOP -->
    <div class="estop-box">
      <button class="btn-estop" id="estopBtn" onclick="triggerEstop()" style="display: none;">
        <span>🛑</span> EMERGENCY STOP
      </button>
      <button class="btn-resume" id="resumeBtn" onclick="resumeEstop()" style="display: flex;">
        <span>⚡</span> RESET & RESUME MOTORS
      </button>
    </div>

    <!-- ULTRASONIC SENSOR RADAR CARD -->
    <div class="card" id="radarCard">
      <div class="card-title" style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
        <span>4-Axis Ultrasonic Radar</span>
        <span id="obstacleAlert" style="color: var(--emerald-neon); font-size: 11px; font-weight: 700; letter-spacing: 0.5px;">PATH CLEAR</span>
      </div>

      <div class="sensor-compass">
        <!-- FRONT S0 -->
        <div class="sensor-node" id="nodeFront">
          <span class="sensor-dir">▲ FRONT</span>
          <span class="sensor-val" id="valFront">---</span>
          <div class="sensor-meter"><div class="meter-bar" id="barFront"></div></div>
        </div>

        <!-- MID ROW: LEFT S3, CENTER ROBOT ICON, RIGHT S1 -->
        <div class="sensor-mid-row">
          <div class="sensor-node" id="nodeLeft">
            <span class="sensor-dir">◀ LEFT</span>
            <span class="sensor-val" id="valLeft">---</span>
            <div class="sensor-meter"><div class="meter-bar" id="barLeft"></div></div>
          </div>

          <div class="robot-center-icon">
            <div class="robot-pulse"></div>
            <span>🤖</span>
          </div>

          <div class="sensor-node" id="nodeRight">
            <span class="sensor-dir">RIGHT ▶</span>
            <span class="sensor-val" id="valRight">---</span>
            <div class="sensor-meter"><div class="meter-bar" id="barRight"></div></div>
          </div>
        </div>

        <!-- BACK S2 -->
        <div class="sensor-node" id="nodeBack">
          <div class="sensor-meter"><div class="meter-bar" id="barBack"></div></div>
          <span class="sensor-val" id="valBack">---</span>
          <span class="sensor-dir">▼ BACK</span>
        </div>
      </div>
    </div>

    <!-- MAIN CONTROLS CARD -->
    <div class="card disabled-overlay" id="controlsCard">
      <div class="card-title">Control Mode</div>
      <div class="mode-grid">
        <button class="btn-mode" id="btnModeAuto" onclick="setMode('AUTO_EVADE')">AUTO EVADE</button>
        <button class="btn-mode active" id="btnModeWeb" onclick="setMode('WEB_OVERRIDE')">MANUAL</button>
        <button class="btn-mode" id="btnModePi" onclick="setMode('PI_OVERRIDE')">PI LINK</button>
      </div>

      <!-- TANK DPAD -->
      <div class="dpad-container" id="dpadPanel">
        <button class="ctrl-btn" onpointerdown="sendMove('pivot_left')" onpointerup="sendMove('stop')">◤</button>
        <button class="ctrl-btn" onpointerdown="sendMove('forward')" onpointerup="sendMove('stop')">▲</button>
        <button class="ctrl-btn" onpointerdown="sendMove('pivot_right')" onpointerup="sendMove('stop')">◥</button>
        
        <button class="ctrl-btn" onpointerdown="sendMove('left')" onpointerup="sendMove('stop')">◀</button>
        <button class="ctrl-btn stop-btn" onclick="sendMove('stop')">STOP</button>
        <button class="ctrl-btn" onpointerdown="sendMove('right')" onpointerup="sendMove('stop')">▶</button>

        <button class="ctrl-btn" style="visibility:hidden;"></button>
        <button class="ctrl-btn" onpointerdown="sendMove('backward')" onpointerup="sendMove('stop')">▼</button>
        <button class="ctrl-btn" style="visibility:hidden;"></button>
      </div>

      <!-- SLIDERS -->
      <div class="slider-group">
        <div class="slider-label">
          <span>Drive Speed</span>
          <span id="speedDisplay">180</span>
        </div>
        <input type="range" id="speedRange" min="80" max="255" value="180" oninput="updateSpeed(this.value)">
      </div>

      <div class="slider-group">
        <div class="slider-label">
          <span>Evade Threshold</span>
          <span id="threshDisplay">25 cm</span>
        </div>
        <input type="range" id="threshRange" min="10" max="60" value="25" oninput="updateThresh(this.value)">
      </div>
    </div>
  </div>

  <script>
    let activeMode = 'WEB_OVERRIDE';
    let isEstop = true;

    async function fetchStatus() {
      try {
        const res = await fetch('/api/status');
        if (!res.ok) return;
        const data = await res.json();

        isEstop = data.estop || false;
        activeMode = data.mode || activeMode;

        // E-Stop UI toggle
        const estopBtn = document.getElementById('estopBtn');
        const resumeBtn = document.getElementById('resumeBtn');
        const controlsCard = document.getElementById('controlsCard');
        const badge = document.getElementById('systemBadge');

        if (isEstop) {
          estopBtn.style.display = 'none';
          resumeBtn.style.display = 'flex';
          controlsCard.classList.add('disabled-overlay');
          badge.innerText = 'E-STOPPED';
          badge.classList.add('estop-alert');
        } else {
          estopBtn.style.display = 'flex';
          resumeBtn.style.display = 'none';
          controlsCard.classList.remove('disabled-overlay');
          badge.innerText = activeMode === 'AUTO_EVADE' ? 'AUTO EVADE' : (activeMode === 'PI_OVERRIDE' ? 'PI CONTROL' : 'MANUAL');
          badge.classList.remove('estop-alert');
        }

        // Mode button styling
        document.getElementById('btnModeAuto').classList.toggle('active', activeMode === 'AUTO_EVADE');
        document.getElementById('btnModeWeb').classList.toggle('active', activeMode === 'WEB_OVERRIDE');
        document.getElementById('btnModePi').classList.toggle('active', activeMode === 'PI_OVERRIDE');

        // Update 4-axis ultrasonic sensor readings
        if (data.d && data.d.length >= 4) {
          const thresh = data.threshold || 25;
          const sensorsData = [
            { id: 'Front', dist: data.d[0] },
            { id: 'Right', dist: data.d[1] },
            { id: 'Back',  dist: data.d[2] },
            { id: 'Left',  dist: data.d[3] }
          ];

          let anyDanger = false;
          let anyWarning = false;

          sensorsData.forEach(s => {
            const node = document.getElementById('node' + s.id);
            const val = document.getElementById('val' + s.id);
            const bar = document.getElementById('bar' + s.id);
            const d = s.dist;

            val.innerText = d >= 300 ? '> 300 cm' : d.toFixed(1) + ' cm';
            const pct = Math.min(100, Math.max(5, (d / 150) * 100));
            bar.style.width = pct + '%';

            node.classList.remove('danger', 'warning');
            if (d < thresh) {
              node.classList.add('danger');
              bar.style.background = 'var(--red-neon)';
              anyDanger = true;
            } else if (d < thresh * 1.5) {
              node.classList.add('warning');
              bar.style.background = '#F59E0B';
              anyWarning = true;
            } else {
              bar.style.background = 'var(--cyan-neon)';
            }
          });

          const alertLbl = document.getElementById('obstacleAlert');
          if (anyDanger) {
            alertLbl.innerText = 'OBSTACLE DETECTED';
            alertLbl.style.color = 'var(--red-neon)';
          } else if (anyWarning) {
            alertLbl.innerText = 'PROXIMITY CAUTION';
            alertLbl.style.color = '#F59E0B';
          } else {
            alertLbl.innerText = 'PATH CLEAR';
            alertLbl.style.color = 'var(--emerald-neon)';
          }
        }
      } catch (err) {
        document.getElementById('systemBadge').innerText = 'OFFLINE';
      }
    }

    function triggerEstop() {
      fetch('/api/estop', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ estop: true })
      }).then(fetchStatus);
    }

    function resumeEstop() {
      fetch('/api/estop', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ estop: false })
      }).then(fetchStatus);
    }

    function sendMove(dir) {
      if (isEstop) return;
      if (activeMode !== 'WEB_OVERRIDE') {
        setMode('WEB_OVERRIDE');
      }
      fetch('/api/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: dir })
      });
    }

    function setMode(mode) {
      if (isEstop) return;
      activeMode = mode;
      fetch('/api/mode', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: mode })
      }).then(fetchStatus);
    }

    function updateThresh(val) {
      document.getElementById('threshDisplay').innerText = val + ' cm';
      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ threshold: parseFloat(val) })
      });
    }

    function updateSpeed(val) {
      document.getElementById('speedDisplay').innerText = val;
      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ speed: parseInt(val) })
      });
    }

    // Keyboard support
    window.addEventListener('keydown', (e) => {
      if (e.repeat || isEstop) return;
      if (e.key === 'ArrowUp' || e.key === 'w') sendMove('forward');
      else if (e.key === 'ArrowDown' || e.key === 's') sendMove('backward');
      else if (e.key === 'ArrowLeft' || e.key === 'a') sendMove('left');
      else if (e.key === 'ArrowRight' || e.key === 'd') sendMove('right');
      else if (e.key === ' ') triggerEstop();
    });

    window.addEventListener('keyup', (e) => {
      if (isEstop) return;
      if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'w', 'a', 's', 'd'].includes(e.key)) {
        sendMove('stop');
      }
    });

    setInterval(fetchStatus, 200);
    fetchStatus();
  </script>
</body>
</html>
)rawliteral";

WebAdminManager::WebAdminManager()
    : server(WEB_SERVER_PORT),
      activeMode(MODE_WEB_OVERRIDE),
      lastWebCmdTime(0),
      wifiWasConnected(false),
      lastLedBlinkTime(0),
      ledState(false),
      lastDisconnectAlertTime(0) {}

void WebAdminManager::init() {
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, LOW);

    Serial.println("[WiFi] Starting Access Point mode...");
    WiFi.mode(WIFI_AP);

    // Launch SoftAP directly
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    localIPStr = WiFi.softAPIP().toString();
    Serial.printf("[WiFi] Access Point Started!\n");
    Serial.printf("       SSID:     %s\n", AP_SSID);
    Serial.printf("       Password: %s\n", AP_PASSWORD);
    Serial.printf("       URL:      http://%s\n", localIPStr.c_str());

    // Configure ArduinoOTA for wireless reprogramming over AP
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        motors.emergencyStop();
        Serial.println("[OTA] Firmware update initiated over AP. Motors cut off.");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\n[OTA] Firmware update complete. Rebooting...");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Error[%u]\n", error);
    });
    ArduinoOTA.begin();
    Serial.println("[OTA] ArduinoOTA ready on Access Point.");

    setupRoutes();
    server.begin();
    Serial.printf("[WebAdmin] Controls-only portal running on http://%s\n", localIPStr.c_str());
}

void WebAdminManager::setupRoutes() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    server.on("/api/control", HTTP_POST, [this]() { handleApiControl(); });
    server.on("/api/estop", HTTP_POST, [this]() { handleApiEstop(); });
    server.on("/api/mode", HTTP_POST, [this]() { handleApiMode(); });
    server.on("/api/config", HTTP_POST, [this]() { handleApiConfig(); });

    server.onNotFound([]() {
        webAdmin.server.send(404, "text/plain", "Not Found");
    });
}

void WebAdminManager::handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void WebAdminManager::handleApiStatus() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<384> doc;
#endif

    if (activeMode == MODE_PI_OVERRIDE) doc["mode"] = "PI_OVERRIDE";
    else if (activeMode == MODE_WEB_OVERRIDE) doc["mode"] = "WEB_OVERRIDE";
    else doc["mode"] = "AUTO_EVADE";

    doc["estop"] = motors.isEmergencyStopped();
    doc["threshold"] = evasion.getThreshold();
    doc["speed"] = motors.getBaseSpeed();

    JsonArray d = doc.createNestedArray("d");
    for (int i = 0; i < NUM_ULTRASONIC_SENSORS; i++) {
        d.add(round(sensors.getDistance(i) * 10.0f) / 10.0f);
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebAdminManager::handleApiEstop() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    bool trigger = doc["estop"] | false;
    if (trigger) {
        motors.emergencyStop();
    } else {
        motors.resetEmergencyStop();
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiControl() {
    if (motors.isEmergencyStopped()) {
        server.send(403, "application/json", "{\"error\":\"E-STOP ACTIVE\"}");
        return;
    }

    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    String action = doc["action"] | "stop";
    activeMode = MODE_WEB_OVERRIDE;
    commPi.setPiOverride(false);
    lastWebCmdTime = millis();

    if (action == "forward") motors.forward();
    else if (action == "backward") motors.backward();
    else if (action == "left") motors.rotateLeft();
    else if (action == "right") motors.rotateRight();
    else if (action == "pivot_left") motors.pivotLeft();
    else if (action == "pivot_right") motors.pivotRight();
    else motors.stop();

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiMode() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    String m = doc["mode"] | "AUTO_EVADE";
    if (m == "AUTO_EVADE") {
        activeMode = MODE_AUTO_EVADE;
        commPi.setPiOverride(false);
    } else if (m == "WEB_OVERRIDE") {
        activeMode = MODE_WEB_OVERRIDE;
        commPi.setPiOverride(false);
        motors.stop();
    } else if (m == "PI_OVERRIDE") {
        activeMode = MODE_PI_OVERRIDE;
        commPi.setPiOverride(true);
        motors.stop();
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::handleApiConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Missing body");
        return;
    }

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    StaticJsonDocument<128> doc;
#endif
    deserializeJson(doc, server.arg("plain"));

    if (doc.containsKey("threshold")) {
        float th = doc["threshold"].as<float>();
        evasion.setThreshold(th);
    }
    if (doc.containsKey("speed")) {
        uint8_t spd = doc["speed"].as<uint8_t>();
        motors.setBaseSpeed(spd);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebAdminManager::update() {
    ArduinoOTA.handle();
    server.handleClient();

    uint32_t now = millis();
    bool currentlyConnected = isConnected();

    if (currentlyConnected) {
        // WiFi connected: Blink ESP32 onboard LED (toggle every 250ms)
        if (now - lastLedBlinkTime >= 250) {
            lastLedBlinkTime = now;
            ledState = !ledState;
            digitalWrite(PIN_STATUS_LED, ledState ? HIGH : LOW);
        }

        if (!wifiWasConnected) {
            wifiWasConnected = true;
            motors.resetEmergencyStop();
            Serial.printf("[WiFi] Client connected! (Active stations: %d). Emergency stop reset.\n", WiFi.softAPgetStationNum());
        }
    } else {
        // WiFi disconnected: Turn OFF status LED
        if (ledState) {
            ledState = false;
            digitalWrite(PIN_STATUS_LED, LOW);
        }

        // Trigger Emergency Stop when WiFi is disconnected
        if (wifiWasConnected) {
            wifiWasConnected = false;
            motors.emergencyStop();
            Serial.println("\n>>> [WIFI SAFETY] WiFi connection lost! Activating EMERGENCY STOP. <<<");
        } else if (!motors.isEmergencyStopped() && (now - lastDisconnectAlertTime >= 3000)) {
            lastDisconnectAlertTime = now;
            motors.emergencyStop();
            Serial.println(">>> [WIFI SAFETY] Waiting for WiFi connection. EMERGENCY STOP active. <<<");
        }
    }

    // Auto-stop if in Web manual override and no control input received for > 1.5s
    if (activeMode == MODE_WEB_OVERRIDE && (now - lastWebCmdTime > 1500)) {
        motors.stop();
    }
}

bool WebAdminManager::isConnected() const {
    bool apConnected = (WiFi.getMode() & WIFI_MODE_AP) && (WiFi.softAPgetStationNum() > 0);
    bool staConnected = (WiFi.getMode() & WIFI_MODE_STA) && (WiFi.status() == WL_CONNECTED);
    return apConnected || staConnected;
}

RobotControlMode WebAdminManager::getActiveMode() const {
    return activeMode;
}

void WebAdminManager::setActiveMode(RobotControlMode mode) {
    activeMode = mode;
}

bool WebAdminManager::isWebOverrideActive() const {
    return activeMode == MODE_WEB_OVERRIDE;
}
