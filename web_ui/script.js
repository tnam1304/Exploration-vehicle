const BROKER_URL = 'wss://broker.hivemq.com:8884/mqtt';
const TOPIC_TELEMETRY = 'rover/telemetry';
const TOPIC_CONTROL   = 'rover/control';

const client = mqtt.connect(BROKER_URL);
const tempValElem   = document.getElementById('temp-val');
const distValElem   = document.getElementById('dist-val');
const statusValElem = document.getElementById('status-val');

client.on('connect', () => {
    console.log('✅ MQTT Connected');
    client.subscribe(TOPIC_TELEMETRY);
});

client.on('message', (topic, message) => {
    if (topic === TOPIC_TELEMETRY) {
        try {
            const data = JSON.parse(message.toString());
            if (tempValElem && data.temp !== undefined) tempValElem.innerText = data.temp;
            if (distValElem && data.dist !== undefined) distValElem.innerText = data.dist;

            // Xử lý cảnh báo
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

            // Truyền dữ liệu tọa độ vào Map
            if (data.x !== undefined && data.y !== undefined && data.theta !== undefined) {
                drawMap(data.x, data.y, data.theta);
            }
        } catch (e) {
            console.error("Parse JSON error:", e);
        }
    }
});

// ==========================================
// HỆ THỐNG HÀNG ĐỢI (QUEUE) ĐIỀU KHIỂN CHỐNG DELAY
// ==========================================
let commandQueue = [];
let isSendingCmd = false;

// Đưa lệnh vào hàng đợi
function sendCmd(cmd) {
    if (navigator.vibrate) navigator.vibrate(35);
    
    // Nếu là lệnh STOP ('S'), ta ưu tiên xử lý ngay lập tức và làm sạch hàng đợi để xe dừng khẩn cấp
    if (cmd === 'S') {
        commandQueue = []; 
        publishCommand('S');
        return;
    }

    // Đẩy lệnh vào queue (giới hạn tối đa 3 lệnh để không bị tồn đọng cũ)
    if (commandQueue.length < 3) {
        commandQueue.push(cmd);
    }
    
    processCommandQueue();
}

// Xử lý tuần tự hàng đợi
function processCommandQueue() {
    if (isSendingCmd || commandQueue.length === 0) return;

    isSendingCmd = true;
    const currentCmd = commandQueue.shift();

    publishCommand(currentCmd);

    // Khoảng giãn cách nhỏ giữa các lệnh để xe và mạng không bị nghẽn (chống delay)
    setTimeout(() => {
        isSendingCmd = false;
        processCommandQueue();
    }, 50); 
}

// Thực thi publish qua MQTT
function publishCommand(cmd) {
    if (client && client.connected) {
        client.publish(TOPIC_CONTROL, cmd);
    }
}


// ==========================================
// CAMERA FPV: TỰ ĐỘNG KẾT NỐI LẠI & THÔNG BÁO THEO GIÂY
// ==========================================
let camIpAddress = "";
let reconnectTimer = null;
let notifyInterval = null;
let isCamConnected = false;

function startStream() {
    const ipInput = document.getElementById('cam-ip').value.trim();
    if (!ipInput) return alert("Nhập IP ESP32-CAM!");
    
    camIpAddress = ipInput;
    isCamConnected = false;
    
    // Xóa các tiến trình cũ nếu đang chạy
    if (reconnectTimer) clearTimeout(reconnectTimer);
    if (notifyInterval) clearInterval(notifyInterval);

    attemptConnection();
}

// Hàm thực hiện kết nối camera
function attemptConnection() {
    const imgElem = document.getElementById('camera-feed');
    const statusElem = document.getElementById('cam-status');

    statusElem.innerText = "ĐANG KẾT NỐI...";
    statusElem.className = "cam-offline";

    // Thêm timestamp để tránh cache trình duyệt
    imgElem.src = `http://${camIpAddress}:81/stream?t=${Date.now()}`;

    // Cứ mỗi 1 giây thông báo nếu chưa kết nối được
    if (notifyInterval) clearInterval(notifyInterval);
    notifyInterval = setInterval(() => {
        if (!isCamConnected) {
            statusElem.innerText = "ĐANG KẾT NỐI LẠI...";
            console.log("⏳ Vẫn đang cố gắng kết nối lại với Camera...");
        }
    }, 1000);

    // Sau 2 giây nếu không thành công (hoặc lỗi sự kiện), tiến hành kết nối lại liên tục
    reconnectTimer = setTimeout(() => {
        if (!isCamConnected) {
            console.warn("⚠️ Kết nối quá hạn, đang thử lại...");
            attemptConnection();
        }
    }, 2000);
}

// Khi ảnh load thành công -> Đã kết nối được
function onCamLoad() {
    isCamConnected = true;
    if (notifyInterval) clearInterval(notifyInterval);
    if (reconnectTimer) clearTimeout(reconnectTimer);

    const statusElem = document.getElementById('cam-status');
    statusElem.innerText = "ONLINE";
    statusElem.className = "cam-online";
    console.log("✅ Kết nối Camera thành công!");
}

// Khi gặp lỗi kết nối camera
function onCamError() {
    isCamConnected = false;
    document.getElementById('camera-feed').src = "";
    
    const statusElem = document.getElementById('cam-status');
    statusElem.innerText = "OFFLINE";
    statusElem.className = "cam-offline";

    // Kích hoạt lại cơ chế thử kết nối sau 2 giây
    if (reconnectTimer) clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(() => {
        if (!isCamConnected && camIpAddress) {
            attemptConnection();
        }
    }, 2000);
}

// Chống vuốt nhầm trên điện thoại
document.addEventListener('contextmenu', event => event.preventDefault());


// ==========================================
// THUẬT TOÁN BẢN ĐỒ ODOMETRY 
// ==========================================
const mapCanvas = document.getElementById('odom-map');
const mapCtx = mapCanvas ? mapCanvas.getContext('2d') : null;

let robotPath = []; 
let currentPose = {x: 0, y: 0, theta: 0};
let camZoom = 40; 
let offsetX = 0;  
let offsetY = 0;  

function resizeCanvas() {
    if (mapCanvas) {
        mapCanvas.width = mapCanvas.parentElement.clientWidth;
        mapCanvas.height = mapCanvas.parentElement.clientHeight;
        renderMap();
    }
}
window.addEventListener('resize', resizeCanvas);
setTimeout(resizeCanvas, 300); 

function worldToScreen(wx, wy) {
    let sx = mapCanvas.width / 2 + (wx - offsetX) * camZoom;
    let sy = mapCanvas.height - 20 - (wy - offsetY) * camZoom; 
    return {x: sx, y: sy};
}

function drawMap(x, y, theta) {
    currentPose = {x, y, theta};
    robotPath.push({x, y});
    if (robotPath.length > 2000) robotPath.shift(); 
    renderMap();
}

function renderMap() {
    if (!mapCtx) return;

    if (robotPath.length > 0) {
        let minX = 0, maxX = 0, minY = 0, maxY = 0; 
        robotPath.forEach(p => {
            if (p.x < minX) minX = p.x; if (p.x > maxX) maxX = p.x;
            if (p.y < minY) minY = p.y; if (p.y > maxY) maxY = p.y;
        });

        let wMeters = (maxX - minX) || 1;
        let hMeters = (maxY - minY) || 1;

        let zoomX = (mapCanvas.width - 40) / wMeters; 
        let zoomY = (mapCanvas.height - 40) / hMeters;
        camZoom = Math.min(zoomX, zoomY, 80); 

        offsetX = (minX + maxX) / 2;
        offsetY = (minY + maxY) / 2 - (mapCanvas.height / 2 - 20) / camZoom;
    }

    mapCtx.clearRect(0, 0, mapCanvas.width, mapCanvas.height);

    let start = worldToScreen(0, 0);
    mapCtx.fillStyle = '#ffffff';
    mapCtx.beginPath();
    mapCtx.arc(start.x, start.y, 4, 0, Math.PI*2);
    mapCtx.fill();

    mapCtx.beginPath();
    mapCtx.strokeStyle = 'rgba(0, 229, 255, 0.8)';
    mapCtx.lineWidth = 2.5;
    for (let i = 0; i < robotPath.length; i++) {
        let pt = worldToScreen(robotPath[i].x, robotPath[i].y);
        if (i === 0) mapCtx.moveTo(pt.x, pt.y);
        else mapCtx.lineTo(pt.x, pt.y);
    }
    mapCtx.stroke();    

    let pos = worldToScreen(currentPose.x, currentPose.y);
    mapCtx.save();
    mapCtx.translate(pos.x, pos.y);
    mapCtx.rotate(-currentPose.theta);

    mapCtx.beginPath();
    mapCtx.moveTo(12, 0);  
    mapCtx.lineTo(-8, -8); 
    mapCtx.lineTo(-4, 0);  
    mapCtx.lineTo(-8, 8);  
    mapCtx.closePath();
    mapCtx.fillStyle = '#ff6d00';
    mapCtx.fill();
    mapCtx.restore();
}

let simInterval;
function runTestMap() {
    if(simInterval) clearInterval(simInterval);
    robotPath = [];
    let t = 0; 
    simInterval = setInterval(() => {
        t += 0.05;
        let r = 2 * t; 
        let x = r * Math.sin(t);
        let y = r * Math.cos(t);
        let dx = Math.sin(t) + t * Math.cos(t);
        let dy = Math.cos(t) - t * Math.sin(t);
        let theta = Math.atan2(dy, dx);
        
        drawMap(x, y, theta);
        
        if(t > 20) {
            clearInterval(simInterval);
        }
    }, 50); 
}

// ==========================================
// NHẬN DIỆN GIỌNG NÓI TIẾNG VIỆT
// ==========================================
const voiceTextElem = document.getElementById('voice-text');
let recognition;

if ('webkitSpeechRecognition' in window || 'SpeechRecognition' in window) {
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    recognition = new SpeechRecognition();
    
    recognition.lang = 'vi-VN';
    recognition.continuous = false;
    recognition.interimResults = false;

    recognition.onstart = function() {
        voiceTextElem.innerText = "Đang nghe... 🎧 (Hãy nói lệnh)";
        voiceTextElem.style.color = "#00e5ff";
    };

    recognition.onresult = function(event) {
        const transcript = event.results[0][0].transcript.toLowerCase();
        voiceTextElem.style.color = "#a7ffeb";

        if (transcript.includes('tiến') || transcript.includes('lên')) {
            sendCmd('F');
            voiceTextElem.innerText = `Đã nghe: "${transcript}" ➡️ ĐI THẲNG`;
        } 
        else if (transcript.includes('lùi') || transcript.includes('xuống')) {
            sendCmd('B');
            voiceTextElem.innerText = `Đã nghe: "${transcript}" ➡️ LÙI LẠI`;
        } 
        else if (transcript.includes('trái')) {
            sendCmd('L');
            voiceTextElem.innerText = `Đã nghe: "${transcript}" ➡️ RẼ TRÁI`;
        } 
        else if (transcript.includes('phải')) {
            sendCmd('R');
            voiceTextElem.innerText = `Đã nghe: "${transcript}" ➡️ RẼ PHẢI`;
        } 
        else if (transcript.includes('dừng') || transcript.includes('stop')) {
            sendCmd('S');
            voiceTextElem.innerText = `Đã nghe: "${transcript}" ➡️ DỪNG XE`;
            document.dispatchEvent(new Event('mouseup'));
        } 
        else if (transcript.includes('đỗ') || transcript.includes('đỗ xe')) {
            sendCmd('P');
            voiceTextElem.innerText = `Đã nghe: "${transcript}" ➡️ AUTO PARKING`;
        } 
        else {
            voiceTextElem.innerText = `Không rõ lệnh: "${transcript}". Thử lại!`;
            voiceTextElem.style.color = "#ff5252";
        }
    };

    recognition.onerror = function(event) {
        voiceTextElem.innerText = "Lỗi Micro! Vui lòng cấp quyền Mic trên trình duyệt.";
        voiceTextElem.style.color = "#ff5252";
    };

    recognition.onend = function() {
        setTimeout(() => {
            voiceTextElem.innerText = 'Nói: "Tiến", "Lùi", "Trái", "Phải", "Đỗ xe", "Dừng"...';
            voiceTextElem.style.color = "#aaa";
        }, 4000);
    };
}

function startSpeechRecognition() {
    if (recognition) {
        try {
            recognition.start();
        } catch (e) {
            console.log("Đã bật Micro rồi!");
        }
    } else {
        alert("Trình duyệt không hỗ trợ nhận diện giọng nói!");
    }
}