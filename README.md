# ESP32 Chat Room

Ứng dụng phòng chat thời gian thực chạy trên ESP32, cho phép nhiều người dùng kết nối và trò chuyện với nhau thông qua giao diện web.

## 📋 Tổng quan

ESP32 Chat Room là một dự án IoT sử dụng vi điều khiển ESP32 để tạo một Access Point WiFi độc lập với máy chủ web tích hợp. Người dùng có thể kết nối vào mạng WiFi của ESP32 và truy cập giao diện chat qua trình duyệt web mà không cần kết nối Internet.

## ✨ Tính năng

- **Phòng chat thời gian thực**: Giao tiếp tức thì sử dụng WebSocket
- **Không cần Internet**: Hoạt động hoàn toàn độc lập với Access Point của ESP32
- **Hỗ trợ nhiều người dùng**: Tối đa 4 người dùng đồng thời
- **Giao diện thân thiện**: Giao diện web đơn giản, dễ sử dụng trên cả mobile và desktop
- **Lưu lịch sử chat**: Tin nhắn được lưu vào SPIFFS với cơ chế quản lý dung lượng tự động
- **Hiển thị người dùng online**: Danh sách người dùng trực tuyến được cập nhật theo thời gian thực
- **Tự động kết nối lại**: Tự động kết nối lại khi bị mất kết nối
- **Captive Portal**: Tự động chuyển hướng người dùng đến trang chat

## 🛠️ Yêu cầu phần cứng

- **ESP32 Development Board** (Feather ESP32 hoặc tương thích)
- **Cáp USB** để nạp code và cấp nguồn
- **Nguồn điện**: USB 5V hoặc pin (tùy chọn cho hoạt động di động)

## 📦 Yêu cầu phần mềm

### IDE và Framework
- [PlatformIO](https://platformio.org/) (khuyến nghị) hoặc Arduino IDE
- ESP32 Arduino Core

### Thư viện phụ thuộc
Các thư viện sau sẽ được tự động cài đặt qua PlatformIO:

- **ESPAsyncWebServer**: Máy chủ web bất đồng bộ
- **AsyncTCP**: TCP bất đồng bộ cho ESP32
- **ArduinoJson** v6.21.3: Xử lý dữ liệu JSON
- **SPIFFS**: Hệ thống file để lưu trữ web files và chat log
- **DNSServer**: Hỗ trợ Captive Portal

## 🚀 Cài đặt

### Bước 1: Clone Repository

```bash
git clone https://github.com/drngominhtu/ESP32CHATTINGROOM.git
cd ESP32CHATTINGROOM
```

### Bước 2: Mở Project trong PlatformIO

1. Cài đặt [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) (extension cho VS Code)
2. Mở folder project trong VS Code
3. PlatformIO sẽ tự động tải các dependencies cần thiết

### Bước 3: Upload Filesystem (SPIFFS)

Trước khi nạp code, bạn cần upload các file web (HTML, CSS, JS) vào SPIFFS:

```bash
# Trong PlatformIO Terminal
pio run --target uploadfs
```

Hoặc trong VS Code:
- Nhấn nút PlatformIO trong thanh bên
- Chọn "Platform" → "Upload Filesystem Image"

### Bước 4: Build và Upload Code

```bash
# Build project
pio run

# Upload code vào ESP32
pio run --target upload

# Mở Serial Monitor để xem logs
pio device monitor
```

Hoặc trong VS Code:
- Nhấn nút "Upload" (→) trong thanh dưới
- Nhấn nút "Serial Monitor" (🔌) để xem output

## 📱 Sử dụng

### Kết nối vào Chat Room

1. **Bật ESP32**: Cấp nguồn cho board ESP32
2. **Kết nối WiFi**: 
   - Tìm và kết nối vào mạng WiFi tên `ESP32Chat`
   - Không cần mật khẩu
3. **Mở trình duyệt**:
   - Mở trình duyệt web bất kỳ
   - Truy cập địa chỉ: `http://192.168.4.1`
   - Hoặc để trình duyệt tự động chuyển hướng (Captive Portal)
4. **Nhập tên**: Nhập tên của bạn và nhấn "Join"
5. **Bắt đầu chat**: Gửi và nhận tin nhắn ngay lập tức!

### Các tính năng trong Chat

- **Gửi tin nhắn**: Nhập tin nhắn và nhấn "Send" hoặc phím Enter
- **Xem người dùng online**: Danh sách hiển thị ở đầu trang
- **Tin nhắn hệ thống**: Thông báo khi có người tham gia/rời khỏi
- **Thoát**: Nhấn nút "Exit" để rời phỏng chat

## 🔧 Cấu hình

### Thay đổi thông tin WiFi AP

Mở file `src/main.cpp` và chỉnh sửa:

```cpp
const char* ssid = "ESP32Chat";      // Tên mạng WiFi
const char* password = "";           // Mật khẩu (để trống = không có mật khẩu)
```

### Thay đổi số lượng người dùng tối đa

```cpp
bool apStarted = WiFi.softAP(ssid, password, 1, false, 4);  // 4 = số người dùng tối đa
```

### Thay đổi giới hạn tin nhắn lưu trữ

```cpp
#define MAX_MESSAGES 50      // Số tin nhắn trong buffer
#define MAX_LOG_SIZE 10240   // Kích thước file log tối đa (bytes)
```

## 🏗️ Kiến trúc hệ thống

### Cấu trúc Project

```
ESP32CHATTINGROOM/
├── src/
│   └── main.cpp              # Code chính của ESP32
├── data/
│   ├── index.html            # Giao diện web
│   ├── script.js             # Logic WebSocket client
│   └── style.css             # Styling
├── include/
│   └── README                # Thông tin về thư mục include
├── lib/                      # Thư viện tùy chỉnh
├── test/                     # Unit tests
├── platformio.ini            # Cấu hình PlatformIO
└── README.md                 # File này
```

### Luồng hoạt động

1. **Khởi động**: ESP32 khởi tạo WiFi AP, DNS Server, Web Server, và WebSocket Server
2. **Kết nối WiFi**: Client kết nối vào AP của ESP32
3. **Truy cập Web**: Client truy cập IP 192.168.4.1, server phục vụ file HTML/CSS/JS từ SPIFFS
4. **WebSocket**: Client tạo kết nối WebSocket để giao tiếp real-time
5. **Chat**: Tin nhắn được gửi qua WebSocket và broadcast đến tất cả clients
6. **Lưu trữ**: Tin nhắn được lưu vào SPIFFS với cơ chế trim tự động

### Giao thức WebSocket

**Client → Server:**
```json
// Join room
{"type": "join", "username": "Tên người dùng"}

// Send message
{"type": "message", "username": "Tên người dùng", "text": "Nội dung tin nhắn"}
```

**Server → Client:**
```json
// Broadcast message
{"type": "message", "username": "Tên người dùng", "text": "Nội dung"}

// System notification
{"type": "system", "text": "Thông báo hệ thống"}

// Update user list
{"type": "userlist", "users": ["User1", "User2", "User3"]}
```

## 🔍 Thông tin kỹ thuật

### Cấu hình WiFi
- **Mode**: Access Point (AP)
- **SSID**: ESP32Chat
- **Channel**: 1 (ổn định nhất)
- **IP Address**: 192.168.4.1
- **Subnet**: 255.255.255.0
- **TX Power**: 8.5dBm (thấp để tăng độ ổn định)
- **Max Connections**: 4 clients
- **Power Save**: Disabled

### Tối ưu hóa
- **Bluetooth disabled**: Tránh xung đột với WiFi (cùng băng tần 2.4GHz)
- **DNS Server**: Hỗ trợ Captive Portal redirect
- **Async Server**: Xử lý nhiều request đồng thời
- **Message Buffer**: Cache tin nhắn trong RAM
- **Log Trimming**: Tự động cắt file log khi quá lớn
- **Watchdog**: Tự động kiểm tra và khởi động lại WiFi AP nếu cần

## 🐛 Xử lý sự cố

### ESP32 không tạo được WiFi AP

**Nguyên nhân**: Board chưa được flash đúng hoặc thiếu quyền

**Giải pháp**:
- Kiểm tra kết nối USB
- Thử reset board bằng nút RESET
- Chắc chắn đã chọn đúng board trong `platformio.ini`

### Không thể upload SPIFFS

**Nguyên nhân**: Board đang chạy code và chiếm dụng SPIFFS

**Giải pháp**:
- Giữ nút BOOT khi upload SPIFFS
- Hoặc dùng lệnh: `pio run --target erase` trước rồi upload lại

### WebSocket không kết nối được

**Nguyên nhân**: Client không reach được ESP32 hoặc lỗi CORS

**Giải pháp**:
- Kiểm tra đã kết nối đúng WiFi `ESP32Chat` chưa
- Thử truy cập trực tiếp: `http://192.168.4.1`
- Xóa cache trình duyệt hoặc thử trình duyệt khác
- Kiểm tra Serial Monitor để xem logs

### Tin nhắn không hiển thị

**Nguyên nhân**: File log đầy hoặc SPIFFS hết dung lượng

**Giải pháp**:
- Code đã có cơ chế trim tự động
- Nếu vẫn lỗi, xóa file log: xóa và upload lại SPIFFS

### ESP32 crash hoặc restart liên tục

**Nguyên nhân**: Quá nhiều connections hoặc memory leak

**Giải pháp**:
- Giảm `MAX_MESSAGES` và `MAX_LOG_SIZE`
- Giảm số người dùng tối đa xuống 2-3
- Kiểm tra Free Heap trong Serial Monitor

## 📊 Serial Monitor Output

Khi ESP32 hoạt động bình thường, bạn sẽ thấy:

```
=== ESP32 Chat Room Starting ===
SPIFFS mounted successfully
[WiFi] AP Started
[WiFi] Starting DNS Server...
=== WiFi AP Information ===
SSID: ESP32Chat
IP Address: 192.168.4.1
============================
HTTP server started on port 80
=== Setup Complete ===
[OK] WiFi: 0 clients, WebSocket: 0 clients, Free Heap: 234567 bytes
```

## 🎨 Tùy chỉnh giao diện

Bạn có thể chỉnh sửa các file trong thư mục `data/`:

- **index.html**: Cấu trúc HTML
- **style.css**: Màu sắc, font chữ, layout
- **script.js**: Logic và tương tác

Sau khi chỉnh sửa, nhớ upload lại SPIFFS:
```bash
pio run --target uploadfs
```

## 📝 API Endpoints

- `GET /` - Trang chủ (index.html)
- `GET /style.css` - CSS file
- `GET /script.js` - JavaScript file
- `GET /history` - Lịch sử chat (text file)
- `WS /ws` - WebSocket endpoint

## 🤝 Đóng góp

Mọi đóng góp đều được chào đón! Bạn có thể:

1. Fork repository
2. Tạo branch mới (`git checkout -b feature/TinhNangMoi`)
3. Commit changes (`git commit -m 'Thêm tính năng mới'`)
4. Push to branch (`git push origin feature/TinhNangMoi`)
5. Tạo Pull Request

## 📄 Giấy phép

Dự án này được phân phối dưới giấy phép MIT. Xem file `LICENSE` để biết thêm chi tiết.

## 👤 Tác giả

- GitHub: [@drngominhtu](https://github.com/drngominhtu)

## 🙏 Cảm ơn

Cảm ơn các thư viện open-source:
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
- ESP32 Arduino Core

## 📞 Liên hệ & Hỗ trợ

Nếu bạn gặp vấn đề hoặc có câu hỏi, vui lòng:
- Tạo [Issue](https://github.com/drngominhtu/ESP32CHATTINGROOM/issues) trên GitHub
- Hoặc liên hệ trực tiếp qua GitHub profile

---

**Chúc bạn có trải nghiệm chat vui vẻ với ESP32! 🚀💬**
