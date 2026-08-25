/**
 * @file WebUI.h
 * @brief Mã nguồn giao diện điều khiển Web (Frontend)
 * 
 * Tích hợp HTML5, CSS3 và Vanilla Javascript. Sử dụng AbortController 
 * để chống nghẽn Socket TCP và ưu tiên lệnh dừng khẩn cấp.
 */

#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

/* Lưu trữ tĩnh trong bộ nhớ Flash (PROGMEM) để tiết kiệm RAM */
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>🚀 Trạm Điều Khiển Xe IoT Pro</title>
  <style>
    /* RESET & BASE CSS */
    * { box-sizing: border-box; user-select: none; -webkit-tap-highlight-color: transparent; outline: none; }
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background: #121212; color: #e0e0e0; text-align: center; margin: 0; padding: 5px; overflow-x: hidden; touch-action: manipulation; }
    
    .header-title { width: 100%; text-align: center; }
    h3 { font-size: 16px; margin: 5px 0 10px 0; color: #00e5ff; font-weight: 800; text-transform: uppercase; letter-spacing: 1px;}
    
    /* LAYOUT ĐÁP ỨNG (RESPONSIVE) */
    .main-container { display: flex; flex-direction: column; width: 100%; max-width: 400px; margin: 0 auto; align-items: center; }
    .left-panel, .right-panel { width: 100%; }

    /* CAMERA STREAM */
    .stream-container { width: 100%; border-radius: 8px; overflow: hidden; border: 1px solid #00e5ff; background: #000; position: relative; margin-bottom: 8px;}
    .stream-container img { width: 100%; height: auto; display: block; max-height: 220px; object-fit: contain; }
    
    /* CONTROL BUTTONS */
    .ctrl-group { display: flex; justify-content: space-between; gap: 5px; margin-bottom: 8px;}
    .toggle-btn { flex: 1; padding: 10px 5px; border-radius: 6px; font-weight: bold; font-size: 11px; cursor: pointer; border: none; color: white; transition: all 0.2s;}
    .toggle-btn.on { background: #2e7d32; }
    .toggle-btn.off { background: #c62828; }
    .toggle-btn.mic-btn { background: #1565c0; }
    .toggle-btn:active { transform: scale(0.95); }
    
    /* ANIMATIONS */
    @keyframes pulse { 0% { background: #d50000; } 50% { background: #ff5252; } 100% { background: #d50000; } }
    .recording { animation: pulse 1s infinite !important; }

    /* TELEMETRY DASHBOARD */
    .dashboard { display: grid; grid-template-columns: repeat(4, 1fr); gap: 5px; margin-bottom: 8px; }
    .box { background: #242424; padding: 6px; border-radius: 6px; border: 1px solid #333; }
    .title { font-size: 8px; color: #aaa; font-weight: bold; margin-bottom: 2px; white-space: nowrap; }
    .val { font-size: 15px; font-weight: bold; font-family: monospace; }
    
    /* SPEED CONTROL SLIDER */
    .speed-box { background: #242424; padding: 8px 12px; border-radius: 6px; display: flex; align-items: center; gap: 10px; margin-bottom: 8px;}
    .speed-header { font-size: 11px; font-weight: bold; color: #00e5ff; white-space: nowrap; }
    .slider { -webkit-appearance: none; width: 100%; height: 6px; border-radius: 3px; background: #444; }
    .slider::-webkit-slider-thumb { -webkit-appearance: none; width: 18px; height: 18px; border-radius: 50%; background: #00e5ff; cursor: pointer;}
    
    /* STATUS BAR */
    .status-bar { padding: 6px; border-radius: 6px; font-weight: bold; font-size: 12px; background-color: #1b5e20; color: #a7ffeb; margin-bottom: 8px;}
    
    /* DEAD-RECKONING MAP */
    .map-box { background: #1a1a1a; border: 1px solid #444; border-radius: 8px; padding: 2px; margin-bottom: 12px; cursor: pointer; }
    .map-title { font-size: 10px; color: #ffeb3b; font-weight: bold; margin-bottom: 2px; }
    canvas#mini-canvas { background: #000; border-radius: 6px; width: 100%; height: 100px; display: block; }
    
    /* JOYPAD CONTROLLER */
    .joypad { display: grid; grid-template-columns: repeat(3, 55px); grid-template-rows: repeat(3, 55px); gap: 8px; justify-content: center; margin: 0 auto; }
    .btn { background: #4caf50; border: none; border-radius: 12px; color: white; font-size: 22px; font-weight: bold; box-shadow: 0 5px #1b5e20; cursor: pointer; transition: background 0.1s, transform 0.1s, box-shadow 0.1s; }
    .btn:active { box-shadow: 0 1px #1b5e20; transform: translateY(4px); }
    #btn-S { background: #ef5350; box-shadow: 0 5px #b71c1c; font-size: 14px; }
    #btn-S:active { box-shadow: 0 1px #b71c1c; }
    #btn-F { grid-column: 2; grid-row: 1; } #btn-L { grid-column: 1; grid-row: 2; } #btn-S { grid-column: 2; grid-row: 2; } #btn-R { grid-column: 3; grid-row: 2; } #btn-B { grid-column: 2; grid-row: 3; }

    /* MODAL OVERLAY CHO BẢN ĐỒ */
    #map-modal { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.95); z-index: 1000; flex-direction: column; align-items: center; justify-content: center; }
    #big-canvas { width: 95vw; height: 95vw; max-width: 600px; max-height: 600px; background: #0a0a0a; border: 2px solid #00e5ff; border-radius: 10px; }
    .close-btn { position: absolute; top: 15px; right: 20px; color: white; font-size: 30px; font-weight: bold; cursor: pointer; background: rgba(255,0,0,0.6); width: 40px; height: 40px; border-radius: 20px; line-height: 38px; text-align: center; }
    .clear-btn { margin-top: 20px; padding: 12px 24px; background: #ff9800; border: none; border-radius: 8px; color: white; font-weight: bold; font-size: 15px; cursor: pointer; }

    /* ----------------------------------------------------------------------
       MÀN HÌNH MÁY TÍNH (PC / LAPTOP VIEWPORT)
       ---------------------------------------------------------------------- */
    @media screen and (min-width: 800px) {
      body { padding: 2vh 4vw; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; }
      h3 { font-size: 26px; margin: 0 0 20px 0; letter-spacing: 2px; }
      .main-container { flex-direction: row; max-width: 1400px; width: 100%; gap: 40px; align-items: stretch; justify-content: center;}
      .left-panel { flex: 1.5; display: flex; flex-direction: column; }
      .right-panel { flex: 1; max-width: 550px; display: flex; flex-direction: column; justify-content: space-between; }
      .stream-container { flex: 1; border-width: 3px; border-radius: 16px; margin-bottom: 0; min-height: 500px; display: flex; align-items: center; justify-content: center; background: #050505; }
      .stream-container img { height: 100%; width: 100%; max-height: 75vh; object-fit: contain; border-radius: 13px; }
      .ctrl-group { gap: 15px; margin-bottom: 15px; }
      .toggle-btn { padding: 15px; font-size: 14px; border-radius: 10px; }
      .dashboard { gap: 10px; margin-bottom: 15px; }
      .box { padding: 12px; border-radius: 10px; }
      .title { font-size: 11px; margin-bottom: 6px; }
      .val { font-size: 24px; }
      .speed-box { padding: 15px 20px; margin-bottom: 15px; border-radius: 10px; }
      .speed-header { font-size: 15px; }
      .slider { height: 10px; }
      .slider::-webkit-slider-thumb { width: 26px; height: 26px; }
      .status-bar { padding: 12px; font-size: 16px; margin-bottom: 15px; border-radius: 10px; }
      .map-box { padding: 5px; margin-bottom: 25px; border-radius: 10px; }
      .map-title { font-size: 13px; margin-bottom: 5px; }
      canvas#mini-canvas { height: 180px; } 
      .joypad { grid-template-columns: repeat(3, 85px); grid-template-rows: repeat(3, 85px); gap: 15px; margin: auto auto 0 auto; }
      .btn { font-size: 36px; border-radius: 20px; box-shadow: 0 8px #1b5e20; }
      .btn:active { box-shadow: 0 3px #1b5e20; transform: translateY(5px); }
      #btn-S { font-size: 20px; box-shadow: 0 8px #b71c1c; }
      #btn-S:active { box-shadow: 0 3px #b71c1c; }
    }
  </style>
</head>
<body>
  <div class="header-title"><h3>🛰️ TRẠM ĐIỀU KHIỂN XE TỰ HÀNH IOT</h3></div>

  <div class="main-container">
    <!-- PANEL TRÁI: CAMERA STREAM -->
    <div class="left-panel">
      <div class="stream-container">
        <img id="video-feed" src="" alt="Đang tải Camera Stream...">
      </div>
    </div>

    <!-- PANEL PHẢI: BẢNG ĐIỀU KHIỂN -->
    <div class="right-panel">
      <div class="ctrl-group">
        <button id="btn-pid" class="toggle-btn on" onclick="togglePID()">⚙️ PID: BẬT</button>
        <button id="btn-voice" class="toggle-btn mic-btn" onclick="toggleVoice()">🎤 GIỌNG NÓI</button>
        <button id="btn-flash" class="toggle-btn off" onclick="toggleFlash()">🔦 FLASH: TẮT</button>
      </div>

      <div class="dashboard">
        <div class="box"><div class="title">TỐC ĐỘ (CM/S)</div><span class="val" id="s" style="color:#b2ff59;">0.0</span></div>
        <div class="box"><div class="title">QUÃNG ĐƯỜNG</div><span class="val" id="m" style="color:#ffd54f;">0.0</span></div>
        <div class="box"><div class="title">NHIỆT ĐỘ (°C)</div><span class="val" id="t" style="color:#ff5252;">--</span></div>
        <div class="box"><div class="title">VẬT CẢN (CM)</div><span class="val" id="d" style="color:#00e5ff;">--</span></div>
      </div>

      <div class="speed-box">
        <div class="speed-header">⚡ C.SUẤT DỰ PHÒNG (<span id="speed-display">80</span>%)</div>
        <input type="range" min="20" max="100" value="80" class="slider" oninput="updateSpeed(this.value)">
      </div>

      <div class="status-bar" id="sys-status">✅ HỆ THỐNG AN TOÀN</div>

      <div class="map-box" onclick="openMap()">
        <div class="map-title">🗺️ HÀNH TRÌNH LIVE (Chạm để xem chi tiết)</div>
        <canvas id="mini-canvas" width="600" height="200"></canvas>
      </div>

      <div class="joypad">
        <button class="btn" id="btn-F">▲</button>
        <button class="btn" id="btn-L">◄</button>
        <button class="btn" id="btn-S" onclick="sendCmd('S')">STP</button>
        <button class="btn" id="btn-R">►</button>
        <button class="btn" id="btn-B">▼</button>
      </div>
    </div>
  </div>

  <!-- MODAL BẢN ĐỒ -->
  <div id="map-modal">
    <div class="close-btn" onclick="closeMap()">×</div>
    <h3 style="color:#ffeb3b; margin-bottom: 15px;">BẢN ĐỒ QUỸ ĐẠO (DEAD-RECKONING)</h3>
    <canvas id="big-canvas" width="600" height="600"></canvas>
    <button class="clear-btn" onclick="clearMap()">XÓA DỮ LIỆU ĐƯỜNG ĐI</button>
  </div>

  <script>
    // Khởi tạo luồng Video khi tải trang
    window.addEventListener('load', function() {
      document.getElementById('video-feed').src = 'http://' + window.location.hostname + ':81/stream';
      requestAnimationFrame(renderMap);
    });

    // ==========================================
    // MODULE ĐIỀU KHIỂN THIẾT BỊ (DEVICE CONTROL)
    // ==========================================
    let pidOn = true;
    function togglePID() {
      pidOn = !pidOn;
      let btn = document.getElementById("btn-pid");
      btn.className = pidOn ? "toggle-btn on" : "toggle-btn off";
      btn.innerHTML = pidOn ? "⚙️ PID: BẬT" : "⚙️ PID: TẮT";
      fetch('/cmd?val=' + (pidOn ? 'P' : 'p'));
    }

    let flashOn = false;
    function toggleFlash() {
      flashOn = !flashOn;
      let btn = document.getElementById("btn-flash");
      btn.className = flashOn ? "toggle-btn on" : "toggle-btn off";
      btn.innerHTML = flashOn ? "🔦 FLASH: SÁNG" : "🔦 FLASH: TẮT";
      fetch('/flash?val=' + (flashOn ? '1' : '0'));
    }

    function updateSpeed(val) {
      document.getElementById('speed-display').innerText = val;
      fetch('/speed?val=' + val, { cache: "no-store" });
    }

    // ==========================================
    // MODULE TRUYỀN NHẬN LỆNH (ABORT CONTROLLER)
    // ==========================================
    let activeCmd = 'S';
    let fetchController = new AbortController(); // Quản lý vòng đời của luồng HTTP

    /**
     * @brief Kỹ thuật hủy luồng (Abort) để ép ESP32 nhận lệnh tức thời
     * Bất cứ khi nào có thao tác nhả tay, request cũ đang kẹt sẽ bị trình duyệt
     * chủ động drop để mở đường cho request Stop.
     */
    function sendCmd(cmd) { 
      // Bỏ qua lệnh trùng lặp để tiết kiệm băng thông, trừ lệnh Stop luôn được gửi
      if (cmd === activeCmd && cmd !== 'S') return; 
      activeCmd = cmd;
      
      // Kích hoạt tín hiệu hủy HTTP request cũ
      fetchController.abort(); 
      // Tạo controller mới cho request chuẩn bị gửi
      fetchController = new AbortController();

      fetch('/cmd?val=' + cmd, { 
        cache: "no-store", 
        signal: fetchController.signal // Gắn signal để theo dõi vòng đời
      }).catch(e => {
        // Bỏ qua lỗi DOMException do ta chủ động abort
      });
    }

    // ==========================================
    // MODULE ĐO TỪ XA (TELEMETRY)
    // ==========================================
    function safetySideText(side) {
      if (side === 1) return " BÊN TRÁI";
      if (side === 2) return " BÊN PHẢI";
      if (side === 3) return " HAI BÊN";
      return "";
    }

    function updateSafetyStatus(data) {
      let sBar = document.getElementById('sys-status');
      let side = safetySideText(data.side);
      if (data.warn === 2) {
        sBar.innerText = "🚨 BÁO ĐỘNG QUÁ NHIỆT";
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 12) {
        sBar.innerText = "❌ LỖI PHẢN HỒI ENCODER";
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 11) {
        sBar.innerText = "❌ LỖI/TIMEOUT CẢM BIẾN SONAR";
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 4) {
        sBar.innerText = "🛑 AEB HOLD - CHỜ KHOẢNG CÁCH AN TOÀN";
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 3) {
        sBar.innerText = "🛑 AEB - PHANH KHẨN CẤP PHÍA TRƯỚC";
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 2) {
        sBar.innerText = "⚠️ PRE-BRAKE - ĐANG GIẢM TỐC";
        sBar.style.backgroundColor = "#e65100";
      } else if (data.safety === 1) {
        sBar.innerText = "⚠️ FCW - VẬT CẢN PHÍA TRƯỚC";
        sBar.style.backgroundColor = "#ef6c00";
      } else if (data.safety === 6) {
        sBar.innerText = "🚨 LÙI NGUY HIỂM" + side + " (" +
                         Math.min(data.rearLeft, data.rearRight) + " CM)";
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 5) {
        sBar.innerText = "⚠️ CẢNH BÁO LÙI" + side;
        sBar.style.backgroundColor = "#e65100";
      } else if (data.safety === 10) {
        sBar.innerText = "⚡ NGUY HIỂM PHÍA SAU" + side + " - ĐANG BOOST";
        sBar.style.backgroundColor = "#6a1b9a";
      } else if (data.safety === 9) {
        sBar.innerText = "🚨 VA CHẠM SAU NGUY HIỂM" + side;
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 8) {
        sBar.innerText = "⚠️ CẢNH BÁO VA CHẠM SAU" + side;
        sBar.style.backgroundColor = "#e65100";
      } else if (data.safety === 7) {
        sBar.innerText = "⚠️ VẬT THỂ PHÍA SAU ĐANG ÁP SÁT" + side;
        sBar.style.backgroundColor = "#ef6c00";
      } else {
        sBar.innerText = "✅ HỆ THỐNG AN TOÀN";
        sBar.style.backgroundColor = "#1b5e20";
      }
    }

    /**
     * @brief Giãn chu kỳ Polling để tối ưu băng thông Wi-Fi
     */
    function fetchTelemetry() {
      fetch('/data', { cache: "no-store" })
        .then(r => r.json())
        .then(data => {
          document.getElementById('t').innerText = data.temp;
          document.getElementById('d').innerText = data.dist;
          document.getElementById('s').innerText = data.spd.toFixed(1);
          document.getElementById('m').innerText = data.trav.toFixed(1);
          currentSpd = data.spd; 

          pidOn = data.pid !== 0;
          let pidBtn = document.getElementById("btn-pid");
          pidBtn.className = pidOn ? "toggle-btn on" : "toggle-btn off";
          pidBtn.innerHTML = pidOn ? "⚙️ PID: BẬT" : "⚙️ PID: TẮT";
          
          if(!voiceTimerActive || data.warn === 2 || data.safety !== 0) {
            updateSafetyStatus(data);
          }
          // Tăng chu kỳ lấy mẫu từ 300ms lên 500ms để giảm nghẽn mạng
          setTimeout(fetchTelemetry, 500); 
        })
        .catch(e => {
          document.getElementById('sys-status').innerText = "❌ MẤT KẾT NỐI WIFI";
          setTimeout(fetchTelemetry, 1000); // Thử lại sau 1s nếu lỗi
        });
    }
    fetchTelemetry();

    // ==========================================
    // MODULE BẢN ĐỒ DEAD-RECKONING
    // ==========================================
    let path = [{x: 0, y: 0}];
    let carX = 0, carY = 0;
    let angle = -Math.PI / 2;
    let lastTime = Date.now();
    let currentSpd = 0;

    function updatePhysics() {
      let now = Date.now();
      let dt = (now - lastTime) / 1000.0;
      lastTime = now;
      if (activeCmd === 'L') angle -= 2.5 * dt;
      if (activeCmd === 'R') angle += 2.5 * dt;
      if (currentSpd > 0.5) {
        let distStep = currentSpd * dt;
        if (activeCmd === 'B') distStep = -distStep;
        carX += Math.cos(angle) * distStep;
        carY += Math.sin(angle) * distStep;
        let lastPt = path[path.length-1];
        let d2 = Math.pow(carX - lastPt.x, 2) + Math.pow(carY - lastPt.y, 2);
        if (d2 > 1.0) path.push({x: carX, y: carY}); // Tối ưu: Lọc bớt điểm neo
      }
    }

    function drawCanvas(id) {
      let cvs = document.getElementById(id);
      if(!cvs || cvs.offsetParent === null) return;
      let ctx = cvs.getContext("2d");
      let w = cvs.width; let h = cvs.height;
      ctx.clearRect(0, 0, w, h);
      let offsetX = w/2 - carX; let offsetY = h/2 - carY;

      // Vẽ lưới tọa độ
      ctx.strokeStyle = "#222"; ctx.lineWidth = 1; ctx.beginPath();
      for(let i=0; i<w; i+=30) { ctx.moveTo(i, 0); ctx.lineTo(i, h); }
      for(let j=0; j<h; j+=30) { ctx.moveTo(0, j); ctx.lineTo(w, j); }
      ctx.stroke();

      // Vẽ quỹ đạo hành trình
      ctx.strokeStyle = "#00e5ff"; ctx.lineWidth = 3; ctx.beginPath();
      if(path.length > 0) ctx.moveTo(offsetX + path[0].x, offsetY + path[0].y);
      for(let p of path) { ctx.lineTo(offsetX + p.x, offsetY + p.y); }
      ctx.stroke();

      // Vẽ Icon xe
      ctx.translate(w/2, h/2); ctx.rotate(angle + Math.PI/2);
      ctx.fillStyle = "#ff5252"; ctx.beginPath();
      ctx.moveTo(0, -9); ctx.lineTo(6, 6); ctx.lineTo(-6, 6); ctx.closePath();
      ctx.fill();
      ctx.rotate(-(angle + Math.PI/2)); ctx.translate(-w/2, -h/2);
    }

    function renderMap() {
      updatePhysics();
      drawCanvas('mini-canvas');
      if(document.getElementById('map-modal').style.display === 'flex') drawCanvas('big-canvas');
      requestAnimationFrame(renderMap);
    }
    
    function openMap() { document.getElementById('map-modal').style.display = 'flex'; }
    function closeMap() { document.getElementById('map-modal').style.display = 'none'; }
    function clearMap() { path = [{x: 0, y: 0}]; carX = 0; carY = 0; angle = -Math.PI / 2; }

    // ==========================================
    // GÁN SỰ KIỆN ĐIỀU KHIỂN (EVENTS)
    // ==========================================
    function attachControl(id, cmd) {
      let el = document.getElementById(id);
      if (!el) return;
      let start = (e) => { if(e.cancelable) e.preventDefault(); sendCmd(cmd); };
      let stop = (e) => { if(e.cancelable) e.preventDefault(); sendCmd('S'); };
      el.addEventListener('touchstart', start, { passive: false });
      el.addEventListener('touchend', stop, { passive: false });
      el.addEventListener('mousedown', start);
      el.addEventListener('mouseup', stop);
      el.addEventListener('mouseleave', stop);
    }
    attachControl('btn-F', 'F'); attachControl('btn-B', 'B');
    attachControl('btn-L', 'L'); attachControl('btn-R', 'R');
    document.addEventListener('contextmenu', event => event.preventDefault()); // Chặn Menu chuột phải

    /* XỬ LÝ PHÍM BẤM (KEYBOARD INTERFACE) */
    let keyMap = { 'KeyW': 'F', 'ArrowUp': 'F', 'KeyS': 'B', 'ArrowDown': 'B', 'KeyA': 'L', 'ArrowLeft': 'L', 'KeyD': 'R', 'ArrowRight': 'R' };
    let activeKey = null;
    window.addEventListener('keydown', function(e) {
      if (keyMap[e.code] && activeKey !== e.code) {
        e.preventDefault(); activeKey = e.code;
        let cmd = keyMap[e.code]; sendCmd(cmd);
        let btn = document.getElementById('btn-' + cmd);
        if(btn) { btn.style.transform = 'translateY(5px)'; btn.style.boxShadow = '0 3px #1b5e20'; }
      }
    });
    window.addEventListener('keyup', function(e) {
      if (e.code === activeKey) {
        e.preventDefault(); let cmd = keyMap[e.code]; activeKey = null; sendCmd('S');
        let btn = document.getElementById('btn-' + cmd);
        if(btn) { btn.style.transform = ''; btn.style.boxShadow = ''; }
      }
    });

    /* ==========================================
       MODULE ĐIỀU KHIỂN GIỌNG NÓI (WEB SPEECH API)
       ========================================== */
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    let recognition;
    let isListening = false;
    let voiceTimer = null;
    let voiceTimerActive = false;

    if (SpeechRecognition) {
      recognition = new SpeechRecognition();
      recognition.lang = 'vi-VN'; 
      recognition.continuous = false;
      recognition.interimResults = false;

      recognition.onstart = function() {
        isListening = true;
        let btn = document.getElementById('btn-voice');
        btn.innerHTML = "🔴 ĐANG NGHE...";
        btn.classList.add('recording');
      };

      recognition.onresult = function(event) {
        let transcript = event.results[0][0].transcript.toLowerCase();
        processVoiceCommand(transcript);
      };

      recognition.onerror = function(event) {
        if (event.error === 'not-allowed') alert("Lỗi: Yêu cầu cấp quyền Micro trên trình duyệt!");
        stopListeningUI();
      };

      recognition.onend = function() { stopListeningUI(); };
    }

    function toggleVoice() {
      if (!SpeechRecognition) {
        alert("Thiếu API hỗ trợ! Yêu cầu sử dụng Google Chrome bản mới nhất.");
        return;
      }
      if (isListening) recognition.stop();
      else recognition.start();
    }

    function stopListeningUI() {
      isListening = false;
      let btn = document.getElementById('btn-voice');
      btn.innerHTML = "🎤 GIỌNG NÓI";
      btn.classList.remove('recording');
    }

    function processVoiceCommand(text) {
      clearTimeout(voiceTimer); 
      let cmd = 'S';
      let actionText = "Dừng lại";
      
      // Khớp từ khóa logic lệnh
      if (text.includes("tiến") || text.includes("lên")) { cmd = 'F'; actionText = "Tiến lên"; }
      else if (text.includes("lùi") || text.includes("xuống")) { cmd = 'B'; actionText = "Đi lùi"; }
      else if (text.includes("trái")) { cmd = 'L'; actionText = "Rẽ trái"; }
      else if (text.includes("phải")) { cmd = 'R'; actionText = "Rẽ phải"; }
      else if (text.includes("dừng")) { cmd = 'S'; actionText = "Dừng lại"; }
      
      // Parser thông số thời gian
      let sec = 0;
      let match = text.match(/\d+/);
      if (match) sec = parseInt(match[0]);
      else if (text.includes("một") || text.includes("mốt")) sec = 1;
      else if (text.includes("hai")) sec = 2;
      else if (text.includes("ba")) sec = 3;
      else if (text.includes("bốn")) sec = 4;
      else if (text.includes("năm")) sec = 5;

      let sBar = document.getElementById('sys-status');
      sBar.style.backgroundColor = "#006064";
      voiceTimerActive = true;
      
      if (cmd !== 'S' && sec > 0) {
        sBar.innerText = "🗣️ Lệnh: " + actionText + " trong " + sec + " giây...";
        sendCmd(cmd);
        voiceTimer = setTimeout(() => {
          sendCmd('S');
          sBar.innerText = "✅ Tự động kích hoạt phanh an toàn!";
          sBar.style.backgroundColor = "#1b5e20";
          voiceTimerActive = false;
        }, sec * 1000);
      } else {
        sBar.innerText = "🗣️ Lệnh thủ công: " + actionText;
        sendCmd(cmd);
        setTimeout(() => { voiceTimerActive = false; }, 2000); 
      }
    }
  </script>
</body>
</html>
)=====";

#endif // WEB_UI_H
