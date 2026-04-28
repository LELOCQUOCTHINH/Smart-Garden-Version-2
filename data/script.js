// ==================== WEBSOCKET ====================
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    renderLedTable();
}

function onOpen(event) {
    console.log('Connection opened');
    Send_Data(JSON.stringify({ page: "request_led_config" }));
    Send_Data(JSON.stringify({ page: "request_neo_config" }));
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
        console.log("📤 Gửi:", data);
    } else {
        console.warn("⚠️ WebSocket chưa sẵn sàng!");
        alert("⚠️ WebSocket chưa kết nối!");
    }
}

function onMessage(event) {
    console.log("📩 Nhận dữ liệu từ ESP32: ", event.data);
    try {
        var data = JSON.parse(event.data);
        // Có thể thêm xử lý riêng nếu cần (ví dụ cập nhật trạng thái)
        if (data.temperature !== undefined && gaugeTemp) {
            gaugeTemp.refresh(data.temperature);
        }
        if (data.humidity !== undefined && gaugeHumi) {
            gaugeHumi.refresh(data.humidity);
        }
        if (data.soil_moisture !== undefined && gaugeSoil) {
            gaugeSoil.refresh(data.soil_moisture);
        }

        if (data.page === "led_config_data") {
            currentLedStates = data.value; // Nạp dữ liệu từ FS của ESP32 vào mảng JS
            renderLedTable();              // Cập nhật lại giao diện bảng HTML
        }

        if (data.page === "neo_config_data") {
            currentNeoStates = data.value;
            renderNeoTable();
        }
    } catch (e) {
        console.warn("Không phải JSON hợp lệ:", event.data);
    }
}


// ==================== UI NAVIGATION ====================
let relayList = [];
let deleteTarget = null;

function showSection(id, event) {
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');
    document.getElementById(id).style.display = id === 'settings' ? 'flex' : 'block';
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');
}


// ==================== HOME GAUGES ====================
var gaugeTemp;
var gaugeHumi;
var gaugeSoil;

window.onload = function () {
    gaugeTemp = new JustGage({
        id: "gauge_temp",
        value: 26,
        min: -10,
        max: 50,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#00BCD4", "#4CAF50", "#FFC107", "#F44336"]
    });

    gaugeHumi = new JustGage({
        id: "gauge_humi",
        value: 60,
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#42A5F5", "#00BCD4", "#0288D1"]
    });

    gaugeSoil = new JustGage({
        id: "gauge_soil",     // Phải khớp với id="gauge_soil" bên file HTML
        value: 0,
        min: 0,
        max: 4095,            // Thang đo ADC của ESP32
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        // Dải màu: Đỏ (Rất khô) -> Vàng (Hơi khô) -> Xanh lá (Tốt) -> Xanh lơ (Sũng nước)
        levelColors: ["#F44336", "#FFC107", "#4CAF50", "#00BCD4"]
    });
};


// ==================== DEVICE FUNCTIONS ====================
function openAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'flex';
}
function closeAddRelayDialog() {
    document.getElementById('addRelayDialog').style.display = 'none';
}
function saveRelay() {
    const name = document.getElementById('relayName').value.trim();
    const gpio = document.getElementById('relayGPIO').value.trim();
    if (!name || !gpio) return alert("⚠️ Please fill all fields!");
    relayList.push({ id: Date.now(), name, gpio, state: false });
    renderRelays();
    closeAddRelayDialog();
}
function renderRelays() {
    const container = document.getElementById('relayContainer');
    container.innerHTML = "";
    relayList.forEach(r => {
        const card = document.createElement('div');
        card.className = 'device-card';
        card.innerHTML = `
      <i class="fa-solid fa-bolt device-icon"></i>
      <h3>${r.name}</h3>
      <p>GPIO: ${r.gpio}</p>
      <button class="toggle-btn ${r.state ? 'on' : ''}" onclick="toggleRelay(${r.id})">
        ${r.state ? 'ON' : 'OFF'}
      </button>
      <i class="fa-solid fa-trash delete-icon" onclick="showDeleteDialog(${r.id})"></i>
    `;
        container.appendChild(card);
    });
}
function toggleRelay(id) {
    const relay = relayList.find(r => r.id === id);
    if (relay) {
        relay.state = !relay.state;
        const relayJSON = JSON.stringify({
            page: "device",
            value: {
                name: relay.name,
                status: relay.state ? "ON" : "OFF",
                gpio: relay.gpio
            }
        });
        Send_Data(relayJSON);
        renderRelays();
    }
}
function showDeleteDialog(id) {
    deleteTarget = id;
    document.getElementById('confirmDeleteDialog').style.display = 'flex';
}
function closeConfirmDelete() {
    document.getElementById('confirmDeleteDialog').style.display = 'none';
}
function confirmDelete() {
    relayList = relayList.filter(r => r.id !== deleteTarget);
    renderRelays();
    closeConfirmDelete();
}


// ==================== SETTINGS FORM (BỔ SUNG) ====================
document.getElementById("settingsForm").addEventListener("submit", function (e) {
    e.preventDefault();

    const ssid = document.getElementById("ssid").value.trim();
    const password = document.getElementById("password").value.trim();
    const token = document.getElementById("token").value.trim();
    const server = document.getElementById("server").value.trim();
    const port = document.getElementById("port").value.trim();
    const localServer = document.getElementById("localServer").value.trim();

    const settingsJSON = JSON.stringify({
        page: "setting",
        value: {
            ssid: ssid,
            password: password,
            token: token,
            server: server,
            port: port,
            local_server: localServer
        }
    });

    Send_Data(settingsJSON);
    alert("✅ Cấu hình đã được gửi đến thiết bị!");
});

// Mảng lưu trữ trạng thái tạm thời trên giao diện
let currentLedStates = [];

function renderLedTable() {
    const tbody = document.getElementById("ledTableBody");
    tbody.innerHTML = "";
    
    currentLedStates.forEach((state, index) => {
        const row = document.createElement("tr");
        row.innerHTML = `
            <td><input type="number" id="temp_${index}" value="${state.temp}"></td>
            <td><input type="number" id="int_${index}" value="${state.interval}"></td>
            <td><button class="btn-delete" onclick="removeLedRow(${index})"><i class="fa-solid fa-trash"></i></button></td>
        `;
        tbody.appendChild(row);
    });
}

function addLedRow() {
    if (currentLedStates.length >= 10) {
        alert("Đã đạt giới hạn tối đa 10 trạng thái!");
        return;
    }
    currentLedStates.push({ temp: 40, interval: 1000 }); // Giá trị mặc định khi thêm mới
    renderLedTable();
}

function removeLedRow(index) {
    currentLedStates.splice(index, 1);
    renderLedTable();
}

function saveLedConfig() {
    // 1. Cập nhật mảng từ các ô input
    currentLedStates.forEach((state, index) => {
        state.temp = parseFloat(document.getElementById(`temp_${index}`).value);
        state.interval = parseInt(document.getElementById(`int_${index}`).value);
    });

    // 2. Sắp xếp mảng tăng dần theo nhiệt độ (Cực kỳ quan trọng để logic C++ chạy đúng)
    currentLedStates.sort((a, b) => a.temp - b.temp);

    // 3. Đóng gói thành JSON chuẩn format bạn đang dùng
    const configJSON = JSON.stringify({
        page: "led_config",
        value: currentLedStates
    });

    // 4. Gửi xuống ESP32
    Send_Data(configJSON);
    alert("Đã gửi cấu hình LED xuống thiết bị!");
    renderLedTable(); // Render lại để hiển thị mảng đã được sắp xếp
}

let currentNeoStates = [];
// Hàm hỗ trợ chuyển đổi từ RGB sang dạng Hex (#RRGGBB) cho input color
function rgbToHex(r, g, b) {
    return "#" + (1 << 24 | r << 16 | g << 8 | b).toString(16).slice(1).toUpperCase();
}

// Hàm hỗ trợ chuyển đổi từ Hex (#RRGGBB) sang RGB cho ESP32
function hexToRgb(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : { r: 0, g: 0, b: 0 };
}

function renderNeoTable() {
    const tbody = document.getElementById("neoTableBody");
    tbody.innerHTML = "";
    
    currentNeoStates.forEach((state, index) => {
        const hexColor = rgbToHex(state.r, state.g, state.b);
        const row = document.createElement("tr");
        row.innerHTML = `
            <td><input type="number" id="neoHumi_${index}" value="${state.humi}"></td>
            <td>
                <input type="color" id="neoColor_${index}" value="${hexColor}" style="width: 50px; height: 35px; cursor: pointer; border: none;">
            </td>
            <td><button class="btn-delete" onclick="removeNeoRow(${index})"><i class="fa-solid fa-trash"></i></button></td>
        `;
        tbody.appendChild(row);
    });
}

function addNeoRow() {
    if (currentNeoStates.length >= 10) return;
    // Default màu Trắng
    currentNeoStates.push({ humi: 50, r: 255, g: 255, b: 255 }); 
    renderNeoTable();
}

function removeNeoRow(index) {
    currentNeoStates.splice(index, 1);
    renderNeoTable();
}

function saveNeoConfig() {
    currentNeoStates.forEach((state, index) => {
        state.humi = parseFloat(document.getElementById(`neoHumi_${index}`).value);
        const hex = document.getElementById(`neoColor_${index}`).value;
        const rgb = hexToRgb(hex); // JS tính toán RGB để nhẹ gánh cho ESP32
        state.r = rgb.r;
        state.g = rgb.g;
        state.b = rgb.b;
    });

    // Sắp xếp tăng dần theo độ ẩm
    currentNeoStates.sort((a, b) => a.humi - b.humi);

    Send_Data(JSON.stringify({
        page: "neo_config",
        value: currentNeoStates
    }));
    alert("Đã gửi cấu hình NeoPixel!");
    renderNeoTable();
}