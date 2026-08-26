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
  <title>XE THÁM HIỂM IOT</title>
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
    .toggle-btn.horn-btn { background: #1565c0; }
    .toggle-btn.horn-btn.active { background: #ef6c00; }
    .toggle-btn:active { transform: scale(0.95); }

    /* TELEMETRY DASHBOARD */
    .dashboard { display: grid; grid-template-columns: repeat(5, minmax(0, 1fr)); gap: 5px; margin-bottom: 8px; }
    .box { background: #242424; padding: 6px; border-radius: 6px; border: 1px solid #333; }
    .title { font-size: 8px; color: #aaa; font-weight: bold; margin-bottom: 2px; white-space: nowrap; }
    .val { font-size: 15px; font-weight: bold; font-family: monospace; }

    /* PID TUNING: mặc định thu gọn để không thay đổi bố cục chính */
    .pid-tuning { background: #202020; border: 1px solid #3b3b3b; border-radius: 6px; margin-bottom: 8px; text-align: left; }
    .pid-tuning summary { padding: 7px 9px; color: #80deea; font-size: 10px; font-weight: bold; cursor: pointer; }
    .pid-panel { padding: 0 8px 8px; }
    .pid-row { display: grid; grid-template-columns: 25px 30px 1fr 30px; gap: 5px; align-items: center; margin-top: 5px; }
    .pid-row label { font-size: 11px; font-weight: bold; color: #ddd; }
    .pid-step { height: 28px; border: 0; border-radius: 5px; background: #424242; color: white; font-size: 16px; cursor: pointer; }
    .pid-input { width: 100%; height: 28px; border: 1px solid #555; border-radius: 5px; background: #111; color: #fff; text-align: center; font-family: monospace; user-select: text; }
    .pid-actions { display: flex; gap: 6px; margin-top: 8px; }
    .pid-action { flex: 1; padding: 7px 4px; border: 0; border-radius: 5px; color: white; font-size: 10px; font-weight: bold; cursor: pointer; }
    .pid-apply { background: #2e7d32; }
    .pid-default { background: #546e7a; }
    .pid-state { margin-top: 6px; color: #b0bec5; font-size: 9px; text-align: center; }
    
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
      .ctrl-group { gap: 10px; margin-bottom: 8px; }
      .toggle-btn { padding: 12px; font-size: 13px; border-radius: 10px; }
      .dashboard { gap: 8px; margin-bottom: 8px; }
      .box { padding: 8px; border-radius: 10px; }
      .title { font-size: 10px; margin-bottom: 4px; }
      .val { font-size: 20px; }
      .pid-tuning { margin-bottom: 8px; border-radius: 10px; }
      .pid-tuning summary { padding: 7px 10px; font-size: 12px; }
      .pid-panel { padding: 0 12px 12px; }
      .pid-row { grid-template-columns: 35px 40px 1fr 40px; gap: 8px; }
      .pid-step, .pid-input { height: 34px; }
      .speed-box { padding: 10px 15px; margin-bottom: 8px; border-radius: 10px; }
      .speed-header { font-size: 13px; }
      .slider { height: 10px; }
      .slider::-webkit-slider-thumb { width: 26px; height: 26px; }
      .status-bar { padding: 9px; font-size: 14px; margin-bottom: 8px; border-radius: 10px; }
      .map-box { padding: 4px; margin-bottom: 10px; border-radius: 10px; }
      .map-title { font-size: 13px; margin-bottom: 5px; }
      canvas#mini-canvas { height: 120px; } 
      .joypad { grid-template-columns: repeat(3, 70px); grid-template-rows: repeat(3, 70px); gap: 10px; margin: auto auto 0 auto; }
      .btn { font-size: 36px; border-radius: 20px; box-shadow: 0 8px #1b5e20; }
      .btn:active { box-shadow: 0 3px #1b5e20; transform: translateY(5px); }
      #btn-S { font-size: 20px; box-shadow: 0 8px #b71c1c; }
      #btn-S:active { box-shadow: 0 3px #b71c1c; }
    }

    @media screen and (max-width: 799px) {
      .stream-container img { max-height: 180px; }
      .pid-tuning { margin-bottom: 5px; }
      .pid-tuning summary { padding: 5px 7px; }
      .dashboard { margin-bottom: 5px; }
      .box { padding: 4px; }
      .title { font-size: 7px; }
      .speed-box { padding: 6px 9px; margin-bottom: 5px; }
      .status-bar { padding: 5px; margin-bottom: 5px; }
      .map-box { margin-bottom: 8px; }
      canvas#mini-canvas { height: 70px; }
      .joypad { grid-template-columns: repeat(3, 50px); grid-template-rows: repeat(3, 50px); gap: 6px; }
    }
  </style>
</head>
<body>
  <div class="header-title"><h3>🛰️ XE THÁM HIỂM IOT</h3></div>

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
        <button id="btn-horn" class="toggle-btn horn-btn">📢 CÒI</button>
        <button id="btn-flash" class="toggle-btn off" onclick="toggleFlash()">🔦 FLASH: TẮT</button>
      </div>


      <details class="pid-tuning">
        <summary>⚙️ HIỆU CHỈNH Kp / Ki / Kd</summary>
        <div class="pid-panel">
          <div class="pid-row">
            <label for="pid-kp">Kp</label>
            <button class="pid-step" onclick="adjustPid('kp', -0.10)">−</button>
            <input id="pid-kp" class="pid-input" type="number" min="0" max="20" step="0.10" value="8.00" oninput="markPidDirty()">
            <button class="pid-step" onclick="adjustPid('kp', 0.10)">+</button>
          </div>
          <div class="pid-row">
            <label for="pid-ki">Ki</label>
            <button class="pid-step" onclick="adjustPid('ki', -0.05)">−</button>
            <input id="pid-ki" class="pid-input" type="number" min="0" max="10" step="0.05" value="2.00" oninput="markPidDirty()">
            <button class="pid-step" onclick="adjustPid('ki', 0.05)">+</button>
          </div>
          <div class="pid-row">
            <label for="pid-kd">Kd</label>
            <button class="pid-step" onclick="adjustPid('kd', -0.01)">−</button>
            <input id="pid-kd" class="pid-input" type="number" min="0" max="5" step="0.01" value="0.00" oninput="markPidDirty()">
            <button class="pid-step" onclick="adjustPid('kd', 0.01)">+</button>
          </div>
          <div class="pid-actions">
            <button class="pid-action pid-apply" onclick="applyPidTunings()">ÁP DỤNG</button>
            <button class="pid-action pid-default" onclick="restoreDefaultPid()">MẶC ĐỊNH</button>
          </div>
          <div id="pid-state" class="pid-state">Đang dùng: Kp 8.00 · Ki 2.00 · Kd 0.00</div>
        </div>
      </details>

      <div class="dashboard">
        <div class="box"><div class="title">TỐC ĐỘ (CM/S)</div><span class="val" id="s" style="color:#b2ff59;">0.0</span></div>
        <div class="box"><div class="title">QUÃNG ĐƯỜNG</div><span class="val" id="m" style="color:#ffd54f;">0.0</span></div>
        <div class="box"><div class="title">NHIỆT ĐỘ (°C)</div><span class="val" id="t" style="color:#ff5252;">--</span></div>
        <div class="box"><div class="title">TRƯỚC (CM)</div><span class="val" id="d" style="color:#00e5ff;">--</span></div>
        <div class="box"><div class="title">SAU (CM)</div><span class="val" id="rear" style="color:#80cbc4;">--</span></div>
      </div>

      <div class="speed-box">
        <div class="speed-header">⚡ MỨC TỐC ĐỘ (<span id="speed-display">50</span>%)</div>
        <input id="speed-slider" type="range" min="20" max="100" value="50" class="slider"
               oninput="previewSpeed(this.value)" onchange="commitSpeed(this.value)"
               onpointerup="finishSpeedControl(this)" onpointercancel="finishSpeedControl(this)">
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
    let pidToggleLocked = false;
    let pendingPidState = null;
    let lastConfirmedPidState = true;
    let pidToggleTimer = null;

    function renderPidButton() {
      let btn = document.getElementById("btn-pid");
      btn.className = pidOn ? "toggle-btn on" : "toggle-btn off";
      btn.innerHTML = pidOn ? "⚙️ PID: BẬT" : "⚙️ PID: TẮT";
    }

    function finishPidToggle(confirmedState) {
      clearTimeout(pidToggleTimer);
      pidToggleTimer = null;
      pidToggleLocked = false;
      pendingPidState = null;
      pidOn = confirmedState;
      lastConfirmedPidState = confirmedState;
      renderPidButton();
    }

    function togglePID() {
      if (pidToggleLocked) return;

      pendingPidState = !pidOn;
      pidToggleLocked = true;
      pidOn = pendingPidState;
      renderPidButton();

      pidToggleTimer = setTimeout(() => {
        finishPidToggle(lastConfirmedPidState);
      }, 1500);

      fetch('/cmd?val=' + (pendingPidState ? 'P' : 'p'), { cache: "no-store" })
        .catch(() => finishPidToggle(lastConfirmedPidState));
    }

    let pidInputsDirty = false;
    let pendingPid = null;

    function markPidDirty() {
      pidInputsDirty = true;
      document.getElementById('pid-state').innerText = "Giá trị mới chưa được áp dụng";
    }

    function adjustPid(name, delta) {
      let input = document.getElementById('pid-' + name);
      let value = parseFloat(input.value);
      if (!Number.isFinite(value)) value = 0;
      value = Math.max(parseFloat(input.min), Math.min(parseFloat(input.max), value + delta));
      input.value = value.toFixed(2);
      markPidDirty();
    }

    function restoreDefaultPid() {
      document.getElementById('pid-kp').value = "8.00";
      document.getElementById('pid-ki').value = "2.00";
      document.getElementById('pid-kd').value = "0.00";
      markPidDirty();
    }

    function applyPidTunings() {
      let state = document.getElementById('pid-state');
      if (activeCmd !== 'S') {
        state.innerText = "Hãy dừng xe trước khi thay đổi PID";
        return;
      }

      let kp = parseFloat(document.getElementById('pid-kp').value);
      let ki = parseFloat(document.getElementById('pid-ki').value);
      let kd = parseFloat(document.getElementById('pid-kd').value);
      if (!Number.isFinite(kp) || !Number.isFinite(ki) || !Number.isFinite(kd) ||
          kp < 0 || kp > 20 || ki < 0 || ki > 10 || kd < 0 || kd > 5) {
        state.innerText = "Giá trị PID không hợp lệ";
        return;
      }

      pendingPid = {
        kp: Math.round(kp * 100),
        ki: Math.round(ki * 100),
        kd: Math.round(kd * 100)
      };
      state.innerText = "Đang gửi xuống STM32...";
      fetch('/pid?kp=' + pendingPid.kp + '&ki=' + pendingPid.ki + '&kd=' + pendingPid.kd,
            { cache: "no-store" })
        .then(r => r.text().then(text => {
          if (!r.ok) throw new Error(text);
          state.innerText = "Đã gửi, đang chờ STM32 xác nhận...";
        }))
        .catch(e => {
          pendingPid = null;
          state.innerText = (e.message === 'STOP_REQUIRED') ?
            "Hãy dừng xe trước khi thay đổi PID" : "Không gửi được hệ số PID";
        });
    }

    function syncPidTelemetry(data) {
      let kp = Number(data.pidKp);
      let ki = Number(data.pidKi);
      let kd = Number(data.pidKd);
      if (!Number.isFinite(kp) || !Number.isFinite(ki) || !Number.isFinite(kd)) return;

      let state = document.getElementById('pid-state');
      let actual = {
        kp: Math.round(kp * 100),
        ki: Math.round(ki * 100),
        kd: Math.round(kd * 100)
      };
      if (pendingPid && actual.kp === pendingPid.kp &&
          actual.ki === pendingPid.ki && actual.kd === pendingPid.kd) {
        pendingPid = null;
        pidInputsDirty = false;
        state.innerText = "Đã áp dụng: Kp " + kp.toFixed(2) +
                          " · Ki " + ki.toFixed(2) + " · Kd " + kd.toFixed(2);
      } else if (!pendingPid && !pidInputsDirty) {
        document.getElementById('pid-kp').value = kp.toFixed(2);
        document.getElementById('pid-ki').value = ki.toFixed(2);
        document.getElementById('pid-kd').value = kd.toFixed(2);
        state.innerText = "Đang dùng: Kp " + kp.toFixed(2) +
                          " · Ki " + ki.toFixed(2) + " · Kd " + kd.toFixed(2);
      }
    }

    let flashOn = false;
    function toggleFlash() {
      flashOn = !flashOn;
      let btn = document.getElementById("btn-flash");
      btn.className = flashOn ? "toggle-btn on" : "toggle-btn off";
      btn.innerHTML = flashOn ? "🔦 FLASH: SÁNG" : "🔦 FLASH: TẮT";
      fetch('/flash?val=' + (flashOn ? '1' : '0'));
    }

    let hornPressed = false;
    let hornPressedAt = 0;
    let hornReleaseTimer = null;
    let hornKeepaliveTimer = null;

    function renderHornButton() {
      let btn = document.getElementById('btn-horn');
      btn.className = hornPressed ? "toggle-btn horn-btn active" : "toggle-btn horn-btn";
      btn.innerHTML = hornPressed ? "📢 CÒI: KÊU" : "📢 CÒI";
    }

    function sendHornState(active) {
      fetch('/horn?val=' + (active ? '1' : '0'), {
        cache: "no-store",
        keepalive: true
      }).catch(() => {});
    }

    function pressHorn() {
      clearTimeout(hornReleaseTimer);
      hornReleaseTimer = null;
      if (hornPressed) return;
      hornPressed = true;
      hornPressedAt = Date.now();
      renderHornButton();
      sendHornState(true);
      hornKeepaliveTimer = setInterval(() => sendHornState(true), 250);
    }

    function releaseHorn() {
      if (!hornPressed) return;
      const remaining = 100 - (Date.now() - hornPressedAt);
      if (remaining > 0) {
        clearTimeout(hornReleaseTimer);
        hornReleaseTimer = setTimeout(releaseHorn, remaining);
        return;
      }
      clearInterval(hornKeepaliveTimer);
      hornKeepaliveTimer = null;
      hornPressed = false;
      renderHornButton();
      sendHornState(false);
    }

    let speedSendTimer = null;
    let speedEditing = false;
    let speedRequestInFlight = false;
    let pendingSpeedValue = null;
    let lastAcknowledgedSpeed = null;

    function pumpSpeedRequest() {
      if (speedRequestInFlight || pendingSpeedValue === null) return;

      const value = pendingSpeedValue;
      if (lastAcknowledgedSpeed !== null && value === lastAcknowledgedSpeed) {
        pendingSpeedValue = null;
        speedEditing = false;
        return;
      }

      speedRequestInFlight = true;
      fetch('/speed?val=' + value, {
        cache: "no-store",
        keepalive: true
      }).then(response => {
        if (response.ok) lastAcknowledgedSpeed = value;
      }).catch(() => {}).finally(() => {
        speedRequestInFlight = false;
        if (pendingSpeedValue === value) pendingSpeedValue = null;
        if (pendingSpeedValue !== null) pumpSpeedRequest();
        else speedEditing = false;
      });
    }

    function queueSpeed(val) {
      clearTimeout(speedSendTimer);
      speedSendTimer = null;
      pendingSpeedValue = Number(val);
      pumpSpeedRequest();
    }

    function previewSpeed(val) {
      speedEditing = true;
      document.getElementById('speed-display').innerText = val;
      clearTimeout(speedSendTimer);
      speedSendTimer = setTimeout(() => queueSpeed(val), 200);
    }

    function commitSpeed(val) {
      speedEditing = true;
      document.getElementById('speed-display').innerText = val;
      queueSpeed(val);
    }

    function finishSpeedControl(slider) {
      commitSpeed(slider.value);
      slider.blur();
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
      activeCmd = cmd;

      // Kích hoạt tín hiệu hủy HTTP request cũ
      fetchController.abort();
      // Tạo controller mới cho request chuẩn bị gửi
      fetchController = new AbortController();
      let requestController = fetchController;

      fetch('/cmd?val=' + cmd, {
        cache: "no-store",
        signal: requestController.signal // Gắn signal để theo dõi vòng đời
      }).catch(e => {
        if (e.name !== 'AbortError' && fetchController === requestController) {
          activeCmd = 'S';
        }
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

    function validSonarValue(value) {
      const distance = Number(value);
      return Number.isFinite(distance) && distance > 0 && distance < 999 ? distance : null;
    }

    function rearDistance(data) {
      const left = validSonarValue(data.rearLeft);
      const right = validSonarValue(data.rearRight);
      if (left === null) return right;
      if (right === null) return left;
      return Math.min(left, right);
    }

    function updateSafetyStatus(data) {
      let sBar = document.getElementById('sys-status');
      let rearCm = rearDistance(data);
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
        sBar.innerText = "🚨 LÙI NGUY HIỂM" +
                         (rearCm === null ? "" : " (" + rearCm + " CM)");
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 5) {
        sBar.innerText = "⚠️ CẢNH BÁO LÙI";
        sBar.style.backgroundColor = "#e65100";
      } else if (data.safety === 10) {
        sBar.innerText = "⚡ NGUY HIỂM PHÍA SAU - ĐANG BOOST";
        sBar.style.backgroundColor = "#6a1b9a";
      } else if (data.safety === 9) {
        sBar.innerText = "🚨 VA CHẠM SAU NGUY HIỂM";
        sBar.style.backgroundColor = "#b71c1c";
      } else if (data.safety === 8) {
        sBar.innerText = "⚠️ CẢNH BÁO VA CHẠM SAU";
        sBar.style.backgroundColor = "#e65100";
      } else if (data.safety === 7) {
        sBar.innerText = "⚠️ VẬT THỂ PHÍA SAU ĐANG ÁP SÁT";
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
          let rearCm = rearDistance(data);
          document.getElementById('rear').innerText = rearCm === null ? "--" : rearCm;
          if (!speedEditing && Number.isFinite(Number(data.speedSet))) {
            lastAcknowledgedSpeed = Number(data.speedSet);
            document.getElementById('speed-slider').value = data.speedSet;
            document.getElementById('speed-display').innerText = data.speedSet;
          }
          updateMapFromEncoders(data.wheelLeft, data.wheelRight);

          let reportedPidState = data.pid !== 0;
          if (pidToggleLocked && reportedPidState === pendingPidState) {
            finishPidToggle(reportedPidState);
          } else if (!pidToggleLocked) {
            pidOn = reportedPidState;
            lastConfirmedPidState = reportedPidState;
            renderPidButton();
          }
          syncPidTelemetry(data);
          
          updateSafetyStatus(data);
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
    const WHEEL_TRACK_CM = 13.5;
    const MAX_ENCODER_STEP_CM = 100.0;
    let path = [{x: 0, y: 0}];
    let carX = 0, carY = 0;
    let angle = -Math.PI / 2;
    let previousEncoderLeft = null;
    let previousEncoderRight = null;

    function updateMapFromEncoders(leftValue, rightValue) {
      const left = Number(leftValue);
      const right = Number(rightValue);
      if (!Number.isFinite(left) || !Number.isFinite(right)) return;

      if (previousEncoderLeft === null || previousEncoderRight === null) {
        previousEncoderLeft = left;
        previousEncoderRight = right;
        return;
      }

      if (Math.abs(left) < 0.2 && Math.abs(right) < 0.2 &&
          (Math.abs(previousEncoderLeft) > 2.0 ||
           Math.abs(previousEncoderRight) > 2.0)) {
        previousEncoderLeft = left;
        previousEncoderRight = right;
        return;
      }

      const dLeft = left - previousEncoderLeft;
      const dRight = right - previousEncoderRight;
      previousEncoderLeft = left;
      previousEncoderRight = right;

      /* Bỏ mẫu nhảy bất thường để nhiễu encoder không kéo hỏng toàn bộ map. */
      if (Math.abs(dLeft) > MAX_ENCODER_STEP_CM ||
          Math.abs(dRight) > MAX_ENCODER_STEP_CM) return;

      const distanceStep = (dLeft + dRight) / 2.0;
      /* Trục Y canvas hướng xuống nên dấu góc đảo so với hệ tọa độ toán học. */
      const angleStep = (dLeft - dRight) / WHEEL_TRACK_CM;
      const middleAngle = angle + angleStep / 2.0;

      carX += distanceStep * Math.cos(middleAngle);
      carY += distanceStep * Math.sin(middleAngle);
      angle += angleStep;
      angle = Math.atan2(Math.sin(angle), Math.cos(angle));

      let lastPt = path[path.length - 1];
      let d2 = Math.pow(carX - lastPt.x, 2) + Math.pow(carY - lastPt.y, 2);
      if (d2 > 1.0) path.push({x: carX, y: carY});
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
      let engaged = false;
      let start = (e) => {
        if (e.cancelable) e.preventDefault();
        if (engaged) return;
        engaged = true;
        if (el.setPointerCapture) el.setPointerCapture(e.pointerId);
        sendCmd(cmd);
      };
      let stop = (e) => {
        if (e.cancelable) e.preventDefault();
        if (!engaged) return;
        engaged = false;
        sendCmd('S');
      };
      el.addEventListener('pointerdown', start);
      el.addEventListener('pointerup', stop);
      el.addEventListener('pointercancel', stop);
      el.addEventListener('lostpointercapture', stop);
    }
    attachControl('btn-F', 'F'); attachControl('btn-B', 'B');
    attachControl('btn-L', 'L'); attachControl('btn-R', 'R');

    let hornButton = document.getElementById('btn-horn');
    hornButton.addEventListener('pointerdown', function(e) {
      if (e.cancelable) e.preventDefault();
      if (hornButton.setPointerCapture) hornButton.setPointerCapture(e.pointerId);
      pressHorn();
    });
    hornButton.addEventListener('pointerup', releaseHorn);
    hornButton.addEventListener('pointercancel', releaseHorn);
    hornButton.addEventListener('lostpointercapture', releaseHorn);

    function releaseControls() {
      if (activeCmd !== 'S') sendCmd('S');
      if (hornPressed) {
        clearTimeout(hornReleaseTimer);
        clearInterval(hornKeepaliveTimer);
        hornReleaseTimer = null;
        hornKeepaliveTimer = null;
        hornPressed = false;
        renderHornButton();
        sendHornState(false);
      }
    }
    window.addEventListener('blur', releaseControls);
    window.addEventListener('pagehide', releaseControls);
    document.addEventListener('contextmenu', event => event.preventDefault()); // Chặn Menu chuột phải

    /* XỬ LÝ PHÍM BẤM (KEYBOARD INTERFACE) */
    let keyMap = { 'KeyW': 'F', 'ArrowUp': 'F', 'KeyS': 'B', 'ArrowDown': 'B', 'KeyA': 'L', 'ArrowLeft': 'L', 'KeyD': 'R', 'ArrowRight': 'R' };
    let activeKey = null;
    window.addEventListener('keydown', function(e) {
      if (e.target && e.target.tagName === 'INPUT') return;
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

  </script>
</body>
</html>
)=====";

#endif // WEB_UI_H
