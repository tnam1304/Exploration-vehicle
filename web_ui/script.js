/**
 * @file script.js
 * @brief Điều khiển xe, nhận dữ liệu MQTT và quản lý luồng Camera FPV.
 */

const BROKER_URL = 'wss://broker.hivemq.com:8884/mqtt';
const TOPIC_TELEMETRY = 'rover/telemetry';
const TOPIC_CONTROL   = 'rover/control';

const client = mqtt.connect(BROKER_URL);

const tempValElem   = document.getElementById('temp-val');
const distValElem   = document.getElementById('dist-val');
const statusValElem = document.getElementById('status-val');

client.on('connect', () => {
    console.log('✅ Kết nối thành công tới MQTT Cloud Broker.');
    if (statusValElem) {
        statusValElem.innerText = '✅ HỆ THỐNG AN TOÀN';
        statusValElem.style.backgroundColor = '#1b5e20';
    }
    client.subscribe(TOPIC_TELEMETRY);
});

client.on('message', (topic, message) => {
    if (topic === TOPIC_TELEMETRY) {
        try {
            const data = JSON.parse(message.toString());
            if (tempValElem && data.temp !== undefined) tempValElem.innerText = data.temp;
            if (distValElem && data.dist !== undefined) distValElem.innerText = data.dist;

            if (statusValElem) {
                if (data.warn === 2) {
                    statusValElem.innerText = '🔥 ALARM CHÁY 🔥';
                    statusValElem.style.backgroundColor = '#b71c1c';
                } else if (data.warn === 1) {
                    statusValElem.innerText = '⚠️ CẢNH BÁO VẬT CẢN';
                    statusValElem.style.backgroundColor = '#e65100';
                } else {
                    statusValElem.innerText = '✅ HỆ THỐNG AN TOÀN';
                    statusValElem.style.backgroundColor = '#1b5e20';
                }
            }
        } catch (e) {
            console.error("Lỗi parse JSON:", e);
        }
    }
});

function sendCmd(cmd) {
    if (navigator.vibrate) navigator.vibrate(35);
    if (client && client.connected) {
        client.publish(TOPIC_CONTROL, cmd);
    }
}

function startSpeechRecognition() {
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SpeechRecognition) {
        alert("Trình duyệt không hỗ trợ nhận diện giọng nói!");
        return;
    }
    const recognition = new SpeechRecognition();
    recognition.lang = "vi-VN";
    
    const voiceText = document.getElementById("voice-text");
    if (voiceText) voiceText.innerText = "🎙️ Đang lắng nghe...";

    recognition.onresult = function(event) {
        const result = event.results[0][0].transcript.toLowerCase();
        if (voiceText) voiceText.innerText = 'Đã nói: "' + result + '"';

        if (result.includes("tiến") || result.includes("chạy")) sendCmd("F");
        else if (result.includes("lùi") || result.includes("sau")) sendCmd("B");
        else if (result.includes("trái")) sendCmd("L");
        else if (result.includes("phải")) sendCmd("R");
        else if (result.includes("dừng") || result.includes("stop")) sendCmd("S");
        else if (result.includes("đỗ")) sendCmd("P");
    };
    recognition.start();
}

function startStream() {
    const ipInput = document.getElementById('cam-ip').value.trim();
    const camFeed = document.getElementById('camera-feed');
    const camStatus = document.getElementById('cam-status');

    if (!ipInput) {
        alert("Nhập IP ESP32-CAM!");
        return;
    }
    camFeed.src = `http://${ipInput}:81/stream`;
    camStatus.innerText = "ONLINE";
    camStatus.className = "cam-online";
}

function onCamError() {
    const camFeed = document.getElementById('camera-feed');
    const camStatus = document.getElementById('cam-status');
    camFeed.src = "";
    camStatus.innerText = "OFFLINE";
    camStatus.className = "cam-offline";
}