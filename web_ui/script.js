/**
 * @file script.js
 * @brief Điều khiển xe qua MQTT Cloud, xử lý nhận diện giọng nói tiếng Việt và quản lý luồng Camera FPV.
 */

// --- CẤU HÌNH MQTT CLOUD ---
const BROKER_URL = 'wss://broker.hivemq.com:8884/mqtt';
const TOPIC_TELEMETRY = 'rover/telemetry';
const TOPIC_CONTROL   = 'rover/control';

// Khởi tạo kết nối MQTT Client
const client = mqtt.connect(BROKER_URL);

// Lấy các element DOM
const tempValElem   = document.getElementById('temp-val');
const distValElem   = document.getElementById('dist-val');
const statusValElem = document.getElementById('status-val');
const voiceText     = document.getElementById("voice-text");

// Sự kiện kết nối MQTT thành công
client.on('connect', () => {
    console.log('✅ Kết nối thành công tới MQTT Cloud Broker.');
    if (statusValElem) {
        statusValElem.innerText = '✅ HỆ THỐNG AN TOÀN';
        statusValElem.style.backgroundColor = '#1b5e20';
    }
    client.subscribe(TOPIC_TELEMETRY);
});

// Nhận dữ liệu Telemetry từ xe đẩy lên
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

// Hàm gửi lệnh điều khiển xe xuống Cloud
function sendCmd(cmd) {
    if (navigator.vibrate) navigator.vibrate(35);
    if (client && client.connected) {
        client.publish(TOPIC_CONTROL, cmd);
        console.log("Command sent via MQTT: " + cmd);
    }
}

// Khởi động nhận diện giọng nói tiếng Việt
function startSpeechRecognition() {
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SpeechRecognition) {
        alert("Trình duyệt không hỗ trợ nhận diện giọng nói! Vui lòng dùng Google Chrome.");
        return;
    }
    const recognition = new SpeechRecognition();
    recognition.lang = "vi-VN";
    recognition.interimResults = false;
    
    if (voiceText) {
        voiceText.innerText = "🎙️ Đang lắng nghe...";
        voiceText.style.color = "#00ffcc";
    }

    recognition.start();

    recognition.onresult = function(event) {
        const result = event.results[0][0].transcript.toLowerCase();
        if (voiceText) {
            voiceText.innerText = 'Đã nói: "' + result + '"';
            voiceText.style.color = "#fff";
        }

        if (result.includes("tiến") || result.includes("thẳng") || result.includes("chạy")) sendCmd("F");
        else if (result.includes("lùi") || result.includes("sau")) sendCmd("B");
        else if (result.includes("trái")) sendCmd("L");
        else if (result.includes("phải")) sendCmd("R");
        else if (result.includes("dừng") || result.includes("stop")) sendCmd("S");
        else if (result.includes("đỗ") || result.includes("parking")) sendCmd("P");
        else {
            if (voiceText) {
                voiceText.innerText += " (Không hiểu lệnh)";
                voiceText.style.color = "#ff5252";
            }
        }
    };

    recognition.onerror = function(event) {
        if (voiceText) {
            voiceText.innerText = "Lỗi mic: " + event.error;
            voiceText.style.color = "#ff5252";
        }
    };
}

// --- QUẢN LÝ LUỒNG CAMERA FPV ---
function startStream() {
    const ipInput = document.getElementById('cam-ip').value.trim();
    const camFeed = document.getElementById('camera-feed');
    const camStatus = document.getElementById('cam-status');

    if (!ipInput) {
        alert("Vui lòng nhập địa chỉ IP của ESP32-CAM!");
        return;
    }
    // Gán đường dẫn stream MJPEG (Mặc định port 81 của ESP32-CAM)
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

/* Chống các hành vi vuốt hoặc giữ menu mặc định trên di động */
document.addEventListener('contextmenu', event => event.preventDefault());
document.addEventListener('touchmove', event => event.preventDefault(), {passive: false});