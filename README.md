# ESP32 Chat Room

Phòng chat thời gian thực sử dụng ESP32 với WiFi Access Point và WebSocket.

## Tính năng

- 🌐 **WiFi Access Point**: ESP32 phát WiFi riêng, không cần router
- 💬 **Chat thời gian thực**: Sử dụng WebSocket để gửi/nhận tin nhắn ngay lập tức
- 👥 **Nhiều người dùng**: Hỗ trợ tối đa 10 người dùng cùng lúc
- 📱 **Giao diện mobile-optimized**: Fix cứng theo màn hình điện thoại, không zoom
- 💾 **Lưu trữ tin nhắn**: Tự động lưu lịch sử chat vào SPIFFS với tính năng ghi đè khi đầy
- 🎨 **UI đơn giản**: Giao diện đen trắng tối giản, không lag
- ⚡ **Tối ưu hiệu năng**: Tắt Bluetooth, giảm công suất phát để ổn định

## Thông số kỹ thuật

- **Board**: Adafruit ESP32 Feather
- **WiFi SSID**: `ESP32Chat`
- **Password**: Không có (open network)
- **IP Address**: `192.168.4.1`
- **WebSocket**: Port 80, path `/ws`
- **Max clients**: 10
- **TX Power**: 11dBm (cân bằng phạm vi và ổn định)
- **Channel**: 1
- **Log size limit**: 10KB (tự động trim khi đầy)

## Cách sử dụng

### 1. Kết nối
1. Bật ESP32
2. Từ điện thoại/laptop, kết nối WiFi `ESP32Chat`
3. Mở trình duyệt, truy cập `http://192.168.4.1`

### 2. Tham gia phòng chat
1. Nhập username (tối đa 50 ký tự)
2. Nhấn **Join**

### 3. Chat
1. Gõ tin nhắn vào ô nhập (tối đa 200 ký tự)
2. Nhấn **Send** hoặc Enter
3. Tin nhắn sẽ hiển thị cho tất cả mọi người

### 4. Thoát
- Nhấn nút **Leave** góc trên phải
- Hoặc tắt trình duyệt/ngắt WiFi

## Cấu trúc dự án

```
ESP32CHATTINGROOM/
├── src/
│   └── main.cpp           # Code ESP32 chính
├── data/
│   ├── index.html         # Giao diện web
│   ├── style.css          # CSS mobile-optimized
│   └── script.js          # WebSocket client
├── platformio.ini         # Cấu hình PlatformIO
└── README.md             # File này
```

## Thư viện sử dụng

- **ESPAsyncWebServer**: HTTP server bất đồng bộ
- **AsyncTCP**: TCP bất đồng bộ cho ESP32
- **ArduinoJson**: Xử lý JSON (parse/serialize)
- **DNSServer**: Captive portal
- **SPIFFS**: File system lưu trữ

## Upload code

### Bằng PlatformIO IDE:
1. Upload filesystem: `Ctrl+Alt+P` → `Upload Filesystem Image`
2. Upload firmware: `Ctrl+Alt+U` hoặc `PlatformIO: Upload`

### Bằng command line:
```bash
# Upload filesystem
pio run --target uploadfs --upload-port COM4

# Upload firmware
pio run --target upload --upload-port COM4
```

## Tối ưu hóa

### WiFi Stability
- ✅ Bluetooth hoàn toàn bị tắt (giải phóng 2.4GHz)
- ✅ TX Power 11dBm (cân bằng giữa phạm vi và ổn định)
- ✅ Channel 1 cố định
- ✅ WiFi sleep mode tắt
- ✅ DNS server cho captive portal
- ✅ Hỗ trợ tối đa 10 clients đồng thời

### Mobile UI
- ✅ `user-scalable=no` - không zoom được
- ✅ `position: fixed` - cố định layout
- ✅ `-webkit-overflow-scrolling: touch` - smooth scroll
- ✅ Font size 16px - tránh auto-zoom iOS
- ✅ Touch-optimized buttons
### Memory Management
- ✅ Log file tự động trim khi vượt 10KB
- ✅ Message buffer giới hạn 50 tin
- ✅ Max 10 clients đồng thời
- ✅ Không có Serial logging (tiết kiệm RAM)
- ✅ Không có Serial logging (tiết kiệm RAM)

## Xử lý sự cố

### Không kết nối được WiFi
- Kiểm tra ESP32 đã khởi động chưa (LED sáng)
- Quét lại WiFi trên điện thoại
- Thử tắt/bật WiFi điện thoại

### Không load được trang web
- Đảm bảo đã kết nối WiFi `ESP32Chat`
- Thử truy cập `192.168.4.1` thay vì tên miền
- Xóa cache trình duyệt

### WebSocket không kết nối
- Kiểm tra JavaScript console (F12)
- Refresh trang (Ctrl+R)
- Ngắt kết nối WiFi và kết nối lại

### Giao diện bị zoom trên điện thoại
- Hard refresh: Ctrl+Shift+R
- Xóa cache trình duyệt
- Thử trình duyệt khác
## Lưu ý

- **Khoảng cách**: Với công suất 11dBm, khoảng cách WiFi khoảng 10-15m
- **Khoảng cách**: Do công suất thấp, khoảng cách WiFi khoảng 5-10m
- **Tin nhắn**: Giới hạn 200 ký tự/tin để tránh quá tải
- **Username**: Giới hạn 50 ký tự
- **Lịch sử chat**: Lưu trong SPIFFS, không mất khi restart (trừ khi erase flash)

## Tác giả

- Phát triển bởi: GitHub Copilot + User
- Board: Adafruit ESP32 Feather
- Ngày tạo: December 2025

## License

MIT License - Tự do sử dụng và chỉnh sửa
