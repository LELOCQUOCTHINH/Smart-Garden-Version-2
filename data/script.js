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
    // Yêu cầu dữ liệu cấu hình LED
    Send_Data(JSON.stringify({ page: "request_led_config" }));
    // Yêu cầu dữ liệu cấu hình NeoPixel
    Send_Data(JSON.stringify({ page: "request_neo_config" }));
    // Yêu cầu dữ liệu cấu hình ngưỡng cảnh báo
    Send_Data(JSON.stringify({ page: "request_threshold_config" }));
    // Yêu cầu dữ liệu cấu hình Relay
    Send_Data(JSON.stringify({ page: "request_relay_config" }));
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

        if (data.state !== undefined && data.state != lastWebState) {
            lastWebState = data.state;
            showAlert(data.state);
        }

        if (data.page === "threshold_config_data") {
            document.getElementById('tw').value = data.value.tw;
            document.getElementById('tc').value = data.value.tc;
            document.getElementById('hw').value = data.value.hw;
            document.getElementById('hc').value = data.value.hc;
            document.getElementById('sw').value = data.value.sw;
            document.getElementById('sc').value = data.value.sc;
        }

        // if (data.page === "threshold_saved") {
        //     alert("✅ Đã lưu ngưỡng cảnh báo xuống bộ nhớ thành công!");
        // }

        if (data.state !== undefined) {
            updateSystemStatus(data.state);
        }

        if (data.page === "relay_config_data") {
            // Xóa mảng cũ
            relayList = []; 
            // Đổ dữ liệu từ ESP32 vào lại mảng JS
            data.value.forEach(item => {
                relayList.push({
                    id: item.id,
                    name: item.name,
                    gpio: item.gpio,
                    mode: item.mode,
                    state: item.state
                });
            });
            // Vẽ lại danh sách Relay
            renderRelays();
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
    const nameInput = document.getElementById('relayName').value;
    const name = nameInput.trim() !== "" ? nameInput : "Thiết bị mới";
    const gpio = parseInt(document.getElementById('relayGPIO').value);
    const mode = parseInt(document.getElementById('relayMode').value);

    if (isNaN(gpio)) {
        alert("Vui lòng nhập số GPIO hợp lệ!");
        return;
    }

    // 1. Cập nhật mảng relayList trên giao diện Web
    const existing = relayList.find(r => r.gpio === gpio);
    if (existing) {
        // Nếu đã có GPIO này rồi thì cập nhật
        existing.name = name;
        existing.mode = mode;
    } else {
        // Nếu chưa có thì thêm mới
        relayList.push({
            id: Date.now(), // Tạo ID ngẫu nhiên trên web để quản lý
            name: name,
            gpio: gpio,
            mode: mode,
            state: 0
        });
    }

    // 2. Gửi lệnh xuống ESP32
    const payload = {
        page: "device",
        action: "update",
        value: { name: name, gpio: gpio, mode: mode, state: 0 }
    };
    Send_Data(JSON.stringify(payload));

    // 3. Vẽ lại danh sách và đóng hộp thoại
    renderRelays();
    closeAddRelayDialog();

    // 4. Reset form cho lần thêm sau
    document.getElementById('relayName').value = "";
    document.getElementById('relayGPIO').value = "";
    document.getElementById('relayMode').value = "0";
}

// Hàm gửi lệnh Bật/Tắt thủ công (chỉ áp dụng nếu mode == 0)
function controlRelay(gpio, state) {
    const payload = {
        page: "device",
        action: "update",
        value: { gpio: gpio, mode: 0, state: state }
    };
    Send_Data(JSON.stringify(payload));
}

function switchMode(id) {
    const relay = relayList.find(r => r.id === id);
    if (relay) {
        // Chuyển đổi qua lại giữa 0 và 1
        relay.mode = (relay.mode === 1) ? 0 : 1;
        
        // Gửi dữ liệu cập nhật xuống ESP32
        const payload = {
            page: "device",
            action: "update",
            value: {
                gpio: relay.gpio,
                mode: relay.mode,
                state: relay.state // Giữ nguyên trạng thái hiện tại
            }
        };
        
        Send_Data(JSON.stringify(payload));
        
        // Vẽ lại giao diện
        renderRelays();
        console.log(`🔄 Đã chuyển GPIO ${relay.gpio} sang chế độ: ${relay.mode === 1 ? 'AI' : 'Manual'}`);
    }
}

function renderRelays() {
    const container = document.getElementById('relayContainer');
    container.innerHTML = "";
    
    relayList.forEach(r => {
        const card = document.createElement('div');
        card.className = 'device-card';
        
        // Xác định badge và trạng thái nút bấm dựa trên mode
        const isAI = (r.mode === 1);
        const modeText = isAI ? "🤖 AI Control" : "✋ Manual";
        const modeClass = isAI ? "mode-ai" : "mode-manual";
        
        card.innerHTML = `
            <i class="fa-solid fa-bolt device-icon"></i>
            <div class="device-info">
                <h3>${r.name}</h3>
                <p>GPIO: ${r.gpio}</p>
            </div>
            
            <div class="mode-status">
                <span class="mode-badge ${modeClass}">${modeText}</span>
                <button class="switch-mode-btn" onclick="switchMode(${r.id})">
                    <i class="fa-solid fa-arrows-rotate"></i> Đổi chế độ
                </button>
            </div>

            <button class="toggle-btn ${r.state ? 'on' : ''}" 
                onclick="toggleRelay(${r.id})" 
                ${isAI ? 'disabled style="opacity: 0.4; cursor: not-allowed;"' : ''}>
                ${r.state ? 'ON' : 'OFF'}
            </button>

            <i class="fa-solid fa-trash delete-icon" onclick="showDeleteDialog(${r.id})"></i>
        `;
        container.appendChild(card);
    });
}
function toggleRelay(id) {
    const relay = relayList.find(r => r.id === id);
    if (relay && relay.mode === 0) { // Chỉ cho phép nếu đang ở Manual
        relay.state = relay.state ? 0 : 1;
        
        const payload = {
            page: "device",
            action: "update",
            value: {
                gpio: relay.gpio,
                mode: relay.mode,
                state: relay.state
            }
        };
        
        Send_Data(JSON.stringify(payload));
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

// ==================== LED CONFIG ====================
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

// ==================== NEO PIXEL CONFIG ====================
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

let lastWebState = 0;

// function showAlert(state) {
//     const toastContainer = document.getElementById('toast-container');
//     if (!toastContainer) return;

//     // Nếu trạng thái là NORMAL (0) thì không hiện cảnh báo
//     if (state === 0) return;

//     // Tạo thẻ div cho thông báo
//     const toast = document.createElement('div');
    
//     // Set màu sắc (2: Đỏ - Critical, 1: Vàng - Warning)
//     toast.className = 'toast ' + (state === 2 ? 'critical' : 'warning');

//     // Nội dung thông báo
//     const message = (state === 2) ? 
//                     "🚨 CRITICAL: Hệ thống vượt ngưỡng nguy hiểm!" : 
//                     "⚠️ WARNING: Hệ thống vượt ngưỡng cảnh báo!";

//     // Đưa HTML vào thông báo (Gồm text và dấu X để tắt)
//     toast.innerHTML = `
//         <span>${message}</span>
//         <button class="toast-close" onclick="closeToast(this)">&times;</button>
//     `;

//     // Thêm vào góc dưới bên phải
//     toastContainer.appendChild(toast);

//     // Tự động tắt sau 5 giây (5000ms)
//     setTimeout(() => {
//         closeToast(toast.querySelector('.toast-close'));
//     }, 5000);
// }

// // Hàm hỗ trợ tắt thông báo với hiệu ứng mờ dần
// function closeToast(buttonElement) {
//     const toast = buttonElement.parentElement;
//     if (toast) {
//         toast.style.animation = 'fadeOutToast 0.4s ease forwards';
//         // Đợi animation chạy xong rồi xóa element khỏi DOM
//         setTimeout(() => {
//             if (toast.parentElement) {
//                 toast.parentElement.removeChild(toast);
//             }
//         }, 400);
//     }
// }

function saveThresholds() {
    const data = {
        page: "threshold_config",
        value: {
            tw: parseFloat(document.getElementById('tw').value),
            tc: parseFloat(document.getElementById('tc').value),
            hw: parseFloat(document.getElementById('hw').value),
            hc: parseFloat(document.getElementById('hc').value),
            sw: parseInt(document.getElementById('sw').value),
            sc: parseInt(document.getElementById('sc').value)
        }
    };
    Send_Data(JSON.stringify(data));
    alert("Đã cập nhật ngưỡng cảnh báo!");
}

function updateSystemStatus(state) {
    const box = document.getElementById('system-status-box');
    const text = document.getElementById('system-status-text');
    const icon = document.getElementById('system-status-icon');
    if (!box) return;

    if (state === 0) {
        box.style.display = 'none'; // Normal -> Ẩn cảnh báo
    } else if (state === 1) {
        box.className = 'status-box warning';
        icon.innerHTML = '⚠️';
        text.innerText = 'WARNING: Hệ thống vượt ngưỡng cảnh báo!';
        box.style.display = 'flex'; // Hiện liên tục
    } else if (state === 2) {
        box.className = 'status-box critical';
        icon.innerHTML = '🚨';
        text.innerText = 'CRITICAL: Hệ thống vượt ngưỡng nguy hiểm!';
        box.style.display = 'flex'; // Hiện liên tục
    }
}