# Test BLE ESP32-C3 bằng điện thoại

Project riêng để kiểm tra BLE của ESP32 cùng công tắc lưỡi gà (reed).

## 0. Đấu dây

| Lưỡi gà | ESP32-C3 |
|---|---|
| Chân 1 | GPIO 4 |
| Chân 2 | GND |

Lưỡi gà 2 chân không phân cực, đấu chiều nào cũng được. Không cần điện trở (dùng pull-up nội).
Nếu là **module lưỡi gà 3 chân**: `S` → GPIO 4, `+` → 3V3, `-` → GND.

## 1. Nạp code

Mở thư mục `helmet/ble_test` bằng VS Code (có extension PlatformIO), cắm ESP32 qua USB:

```bash
cd helmet/ble_test
pio run -t upload
pio device monitor
```

Serial phải in ra:

```
=== ATGT BLE TEST ===
[BLE] Dang quang ba ten: ATGT-TEST
[BLE] MAC: xx:xx:xx:xx:xx:xx
[GUI] dem=0 quai=CHUA CAI (nhip)  (chua co ai ket noi)
```

Chưa cần điện thoại: đưa nam châm lại gần lưỡi gà, Serial phải in `quai=DA CAI (doi)`; kéo ra xa in `quai=CHUA CAI (doi)`.
Nếu bị **ngược** thì đổi `REED_ACTIVE_LOW` thành `0` trong `src/main.cpp`.

> Không nạp được: giữ nút **BOOT**, bấm **RESET**, thả BOOT rồi nạp lại.

## 2. Thử bằng app nRF Connect (Android / iOS)

| Bước | Làm gì | Kết quả đúng |
|---|---|---|
| 1 | Tab **Scanner** → **Scan** | Thấy `ATGT-TEST` |
| 2 | Bấm **Connect** | Serial in `Dien thoai DA KET NOI` |
| 3 | Mở service `f08cbe00-...` | Có 2 characteristic |
| 4 | Ở `f08cbe01-...` bấm nút **notify** (3 mũi tên xuống) | Mỗi giây giá trị đổi: `dem=5 quai=CHUA CAI (nhip)` |
| 5 | Đưa **nam châm** lại gần lưỡi gà | Hiện ngay `quai=DA CAI (doi)`, kéo ra xa hiện `quai=CHUA CAI (doi)` |
| 6 | Ở `f08cbe02-...` bấm **mũi tên lên** (write), chọn kiểu **Text/UTF8**, gửi `1` | LED trên mạch sáng, Serial in `LED BAT` |
| 7 | Gửi `0` | LED tắt |
| 8 | Bấm **Disconnect** | Serial in `MAT KET NOI`, quét lại vẫn thấy `ATGT-TEST` |

Nếu giá trị hiện dạng hex, chạm vào giá trị, đổi hiển thị sang **UTF-8**.

Qua hết 8 bước là BLE của ESP32 ổn, chuyển sang firmware mũ thật ở `helmet/firmware/`.

## Lỗi hay gặp

- **Nam châm lại gần mà không đổi:** kiểm tra dây có cắm đúng GPIO 4 và GND; thử nam châm mạnh hơn hoặc xoay hướng (lưỡi gà nhạy nhất khi nam châm nằm dọc theo thân ống).
- **Báo ngược:** lưỡi gà loại thường đóng (NC) hoặc module 3 chân → đổi `REED_ACTIVE_LOW` thành `0`.
- **Không thấy `ATGT-TEST`:** bật Bluetooth **và Vị trí (Location)** trên Android; xem Serial có dòng `Dang quang ba` không.
- **Serial không in gì:** kiểm tra `-DARDUINO_USB_CDC_ON_BOOT=1` trong `platformio.ini`, rút cắm lại cáp.
- **LED không sáng:** mạch khác có thể không dùng GPIO 8, hoặc LED sáng ở mức HIGH → sửa `LED_PIN` / `LED_ACTIVE_LOW` trong `src/main.cpp`. Không ảnh hưởng tới test BLE.
- **Điện thoại nhớ tên cũ:** trong nRF Connect kéo xuống làm mới, hoặc tắt/bật Bluetooth.
