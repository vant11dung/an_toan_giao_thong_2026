# Kế hoạch: Cảnh báo không cài quai mũ bảo hiểm — Minh

Ngày: 24/11/2026 · Phiên bản 2 (thay cho bản cũ dùng kết nối BLE liên tục) · Trạng thái: **đã viết code (25/11/2026), chờ Minh test trên phần cứng**

---

## 1. Hệ thống làm gì (1 câu)

Mở chìa khoá xe → Orange Pi kiểm tra mũ đã cài quai chưa → chưa cài hoặc mất tín hiệu thì **kêu còi + hiện cảnh báo trên màn hình** trên xe.

```
 MŨ BẢO HIỂM                                   XE MÁY
┌───────────────────────┐                    ┌──────────────────────────────┐
│ Công tắc từ ở quai     │                    │ Orange Pi 4 Pro (nguồn xe)    │
│        │               │   BLE quảng bá     │   ├─ quét BLE, đọc gói của mũ  │
│        ▼               │  ~~~~~~~~~~~~~~>   │   ├─ quyết định OK / cảnh báo  │
│ ESP32-C3 Mini          │   (không kết nối)  │   ├─ còi (module trên xe)      │
│  ngủ sâu, thức dậy     │                    │   └─ màn ST7789 1.54" (SPI)    │
│  phát gói rồi ngủ tiếp │                    └──────────────────────────────┘
│ Pin LiPo 503035 500mAh │
└───────────────────────┘
```

## 2. Phân công: ai làm gì

| Phần | Claude (mình) làm | Minh làm |
|---|---|---|
| Firmware ESP32 trên mũ | Viết code, chú thích rõ từng phần | Đấu dây, nạp code, test với điện thoại |
| Chương trình trên Pi | Viết code đọc BLE, còi, màn hình, tự chạy khi mở chìa | Đấu còi + màn hình, chạy thử trên Pi, báo chân GPIO |
| Tối ưu pin | Tính toán, viết code ngủ sâu | Gỡ đèn nguồn trên mạch, đo dòng bằng đồng hồ |
| Tài liệu hợp đồng | Cập nhật `docs/giao_tiep/mu_bao_hiem_ble.md` | Duyệt, gửi nhóm |

Mỗi bước trong mục 8 đều ghi rõ **ai làm** và **xong khi nào**.

---

## 3. Đã chốt với Minh (24/11/2026)

| Câu hỏi | Chốt |
|---|---|
| Thời gian Pi chờ tín hiệu khi mở chìa | **15 giây** |
| Gửi lại khi đang cài quai | **2 phút đầu sau khi cài: mỗi 10 giây. Sau đó: mỗi 5 phút** |
| Đang chạy mà tháo quai | **Báo ngay** (ESP32 tự thức dậy khi quai mở) |
| Quá 5 phút không nhận tín hiệu | Kêu còi + hiện cảnh báo |
| Cảm biến quai | Công tắc từ (lưỡi gà): cài quai = thông mạch |
| Pin mũ | LiPo **503035, 3.7 V, 500 mAh** (1.85 Wh) |
| Màn hình trên xe | **ST7789 1.54" IPS 240×240, SPI** (chốt 25/11) |
| Còi | Cắm vào GPIO của Pi (loại còi, chân cụ thể: chưa báo) |
| Mạch ESP32 | **ESP32-C3 Super Mini** |
| Sạc pin | **TP4056, đã chỉnh 0.5 A**. Pin nối ESP32 qua **jack 2.54**, hết pin thì rút ra cắm sạc |
| Nút tắt còi tạm | **Không cần** |

---

## 4. Phía mũ (ESP32-C3)

### 4.1 Vì sao đổi từ "kết nối BLE" sang "phát quảng bá"

Bản kế hoạch cũ: mũ và Pi **giữ kết nối** liên tục. Không dùng được nữa vì:

- **Ngủ sâu = tắt hẳn Bluetooth** → kết nối đứt mỗi lần ngủ.
- Mỗi lần thức dậy mà kết nối lại thì mất 1–3 giây, tốn pin gấp nhiều lần.

Cách mới: ESP32 thức dậy → **phát vài gói quảng bá** (giống cái loa hét "tôi đã cài quai!") trong ~0.3 giây → ngủ tiếp.
Pi **quét liên tục** và nghe thấy. Không cần kết nối.

### 4.2 Ba trạng thái của mũ

```
                 cài quai (chân GPIO đổi → thức dậy)
   ┌──────────────┐ ─────────────────────────────> ┌──────────────────────┐
   │  CHƯA CÀI    │                                │  VỪA CÀI (2 phút đầu)│
   │ ngủ sâu hẳn  │ <───────────────────────────── │  thức mỗi 10 giây    │
   │ KHÔNG hẹn giờ│      tháo quai: gửi "chưa cài" │  gửi "đã cài"        │
   └──────────────┘      ngay rồi ngủ hẳn          └──────────────────────┘
          ▲                                                  │ hết 2 phút
          │              tháo quai: gửi "chưa cài"           ▼
          │              ngay rồi ngủ hẳn          ┌──────────────────────┐
          └─────────────────────────────────────── │  ĐANG CÀI            │
                                                   │  thức mỗi 5 phút     │
                                                   │  gửi "đã cài"        │
                                                   └──────────────────────┘
```

- **Chưa cài:** ngủ sâu, **không hẹn giờ**, chỉ thức khi quai được cài. Mũ để trên kệ gần như không tốn pin.
- **Vừa cài:** 2 phút đầu gửi dày (10 giây/lần) để lúc mở chìa, Pi bắt được trong 15 giây.
- **Đang cài:** 5 phút/lần, báo "vẫn đang cài".
- **Tháo quai bất kỳ lúc nào:** ESP32 thức dậy ngay nhờ chân GPIO, gửi "chưa cài", rồi ngủ hẳn.

### 4.3 Mỗi lần thức dậy làm gì

1. Đọc công tắc quai, chờ ~50 ms đọc lại cho chắc (lọc dội).
2. Phát gói quảng bá ~0.3 giây (khoảng 10 gói, phòng Pi bỏ lỡ 1–2 gói).
3. Tính lần thức tiếp theo (10 giây / 5 phút / không hẹn) và đặt chân GPIO để thức khi quai đổi.
4. Ngủ sâu.

Bộ đếm "đã cài được bao lâu" lưu trong **bộ nhớ RTC** (vùng nhớ không mất khi ngủ sâu).

### 4.4 Chân cắm

| Linh kiện | Chân ESP32-C3 | Ghi chú |
|---|---|---|
| Công tắc từ | **GPIO 4** + GND | C3 chỉ cho **GPIO 0–5** đánh thức từ ngủ sâu, nên phải dùng 1 trong các chân này |
| Điện trở kéo lên (**bắt buộc**) | GPIO 4 → 3V3, **10 kΩ** | Lúc ngủ mà đang cài quai, thư viện ESP tự bật kéo **xuống** ~45 kΩ trong chip. Điện trở ngoài phải nhỏ hơn hẳn (10 kΩ) thì tháo quai chân mới lên HIGH để đánh thức chip. 470 kΩ – 1 MΩ **không chạy**. Tháo quai mà không báo ngay thì đổi 4.7 kΩ |
| Pin LiPo | Chân **5V** + GND (qua TP4056 OUT+/OUT−, jack 2.54) | **KHÔNG** cắm thẳng vào 3V3 (pin đầy 4.2 V làm hỏng chip) |

### 4.5 Gói tin quảng bá (dự kiến, chốt trong tài liệu hợp đồng)

Nằm trong phần "Manufacturer Specific Data" của gói quảng bá:

| Byte | Tên | Ý nghĩa |
|---|---|---|
| 0–1 | company ID | `0xFFFF` (mã dành cho thử nghiệm) |
| 2–3 | magic | `'A' 'T'`, để Pi biết đúng là mũ ATGT |
| 4 | version | = 2 |
| 5 | flags | bit0 = đã cài quai |
| 6 | battery | % pin, 255 = chưa đo |
| 7 | seq | Tăng mỗi lần thức. **Bắt buộc phải đổi**: Linux (BlueZ) bỏ qua gói giống hệt gói trước |

Pi chỉ nhận gói từ **đúng MAC của mũ mình** (cấu hình trong Pi), không nhầm với mũ xe khác.

---

## 5. Tính toán năng lượng

> Các số dưới đây là **ước tính** từ datasheet. Bước 3 (mục 8) sẽ đo thật rồi sửa lại.

### 5.1 Thông số

| Mục | Giá trị ước tính |
|---|---|
| Pin dùng được (để chừa ~20% cho pin bền) | 500 × 0.8 = **400 mAh** |
| Chip ESP32-C3 ngủ sâu | ~5 µA |
| IC ổn áp (LDO) trên mạch C3 Super Mini | ~40–50 µA (tuỳ loại mạch) |
| **Đèn nguồn trên mạch** (sáng liên tục) | **~1–3 mA** ← thủ phạm chính |
| Mỗi lần thức (khởi động + phát BLE ~0.4 s, ~35 mA) | ~14 mA·s = **0.004 mAh** |
| Kéo lên ngoài 10 kΩ khi đang cài (công tắc đóng) | 3.3 V / 10 kΩ ≈ **330 µA** |

### 5.2 Kịch bản: đi 2 chuyến/ngày, mỗi chuyến 1 giờ

| Khoản | Cách tính | mAh/ngày |
|---|---|---|
| Thức dậy phát BLE | mỗi chuyến: 12 lần (2 phút đầu) + 12 lần (5 phút/lần) + 2 lần cài/tháo ≈ 26 lần × 2 chuyến × 0.004 | **0.2** |
| Kéo lên 10 kΩ khi đang cài | 330 µA × 2 giờ | **0.7** |
| Ngủ (chip + LDO) | 50 µA × 24 giờ | **1.2** |
| Đèn nguồn nếu **không gỡ** | 2 mA × 24 giờ | **48** |

| Trường hợp | Tổng | Pin 400 mAh dùng được |
|---|---|---|
| Để nguyên mạch (còn đèn nguồn) | ~50 mAh/ngày | **~8 ngày** |
| **Gỡ đèn nguồn** | ~2.1 mAh/ngày | **~6 tháng** (thực tế ít hơn do pin LiPo tự xả ~2–3%/tháng) |

### 5.3 Kết luận

1. **Việc quan trọng nhất là gỡ đèn nguồn trên mạch ESP32.** Số lần thức dậy gần như không ảnh hưởng.
2. Sau đó, phần tốn nhất là **IC ổn áp chờ** (1.2 mAh/ngày). Muốn hơn nữa thì thay mạch ổn áp dòng chờ thấp. Chưa cần làm.
3. Nếu hay **treo mũ lên xe mà vẫn cài quai** thì mũ ở trạng thái "đang cài" cả ngày:
   - Thức 288 lần/ngày ≈ 1.2 mAh, cộng điện trở 10 kΩ: 330 µA × 24 h ≈ 8 mAh → tổng ~10 mAh/ngày, chỉ dùng được **~5–6 tuần**.
   - Không treo mũ mà vẫn cài quai thì không bị khoản này.
4. Code sẽ hạ xung CPU xuống 80 MHz và tắt mọi thứ không cần (Serial, LED) khi chạy bằng pin.

### 5.4 Lưu ý an toàn về pin

- Pin LiPo cần **mạch sạc có bảo vệ** (TP4056 loại 6 chân, có chip DW01). Minh đã chỉnh dòng sạc 0.5 A cho pin 500 mAh.
- Nhiều mạch C3 Super Mini **không có điốt** giữa cổng USB và chân 5V. Cắm USB để nạp code trong khi pin đang nối vào chân 5V thì có thể **đẩy 5V ngược vào pin** → nguy hiểm.
  → **Tháo pin ra trước khi cắm USB nạp code**, cho tới khi kiểm tra sơ đồ mạch của bạn.

---

## 6. Phía xe (Orange Pi 4 Pro)

### 6.1 Ba trạng thái của Pi

```
 mở chìa → Pi khởi động → chương trình chạy
                │
                ▼
        ┌───────────────┐  nhận "đã cài"   ┌────────────┐
        │  CHỜ MŨ       │ ───────────────> │    OK      │
        │  tối đa 15 s  │                  │ màn: "Đã   │
        └───────────────┘                  │  cài quai" │
                │ hết 15 s, chưa nhận      └────────────┘
                ▼                            │       ▲
        ┌───────────────┐ <──────────────────┘       │
        │  CẢNH BÁO     │  - nhận "chưa cài"         │
        │  còi bíp bíp  │  - quá 5 phút 30 giây      │
        │  màn hiện lý  │    không nhận gói nào      │
        │  do cảnh báo  │ ───────────────────────────┘
        └───────────────┘   nhận "đã cài" → tắt còi
```

- 15 giây tính **từ lúc chương trình trên Pi chạy**, không phải từ lúc vặn chìa. Pi khởi động mất khoảng 20–40 giây, sẽ đo ở bước 7.
- Ngưỡng "mất tín hiệu" để **5 phút 30 giây** (5 phút + 30 giây dư), phòng gói đến trễ hoặc bị mất.
- Màn hình hiện **2 loại** lời cảnh báo khác nhau:
  - `CHUA CAI QUAI MU!`: nhận được gói "chưa cài", hoặc hết 15 giây đầu mà không thấy mũ.
  - `MAT TIN HIEU MU!`: đang OK rồi tự nhiên quá 5 phút 30 giây không nhận gì.
- Còi: **bíp ngắt quãng** (0.5 giây kêu / 0.5 giây nghỉ), kêu cho tới khi nhận "đã cài". Muốn kiểu khác thì sửa sau.

### 6.2 Các file trên Pi (mỗi file một việc)

| File | Nhiệm vụ |
|---|---|
| `config.py` | MAC của mũ, chân GPIO còi, các mốc thời gian (15 s, 5 phút 30 giây) |
| `ble_scanner.py` | Quét BLE, lọc đúng mũ, giải mã gói → "đã cài / chưa cài" |
| `helmet_monitor.py` | **Bộ não**: 3 trạng thái ở trên, quyết định khi nào cảnh báo |
| `buzzer.py` | Bật/tắt còi qua GPIO |
| `display.py` | Driver màn ST7789 240×240 qua SPI + vẽ chữ tiếng Việt. `DISPLAY_ENABLED = False` thì chỉ in ra terminal |
| `gpio_line.py` | Bật/tắt chân GPIO qua libgpiod (dùng cho còi, chân DC/RST của màn) |
| `helmet-monitor.service` | Tự chạy chương trình khi Pi khởi động (tức là khi mở chìa) |

---

## 7. Cách xếp thư mục (thay đổi so với bản cũ)

```
an_toan_giao_thong_2026/
├── docs/
│   ├── KE_HOACH_MU_BAO_HIEM.md      # ← file này
│   └── giao_tiep/mu_bao_hiem_ble.md # SẼ CẬP NHẬT: gói quảng bá thay cho GATT
└── helmet/
    ├── ble_test/                    # GIỮ NGUYÊN: test lưỡi gà + BLE với điện thoại
    ├── firmware/                    # SẼ VIẾT LẠI: ngủ sâu + quảng bá
    │   ├── include/config.h         #   chân, mốc thời gian (10 s, 2 phút, 5 phút)
    │   ├── include/helmet_protocol.h#   gói 8 byte ở mục 4.5
    │   └── src/
    │       ├── main.cpp             #   thức dậy → đọc quai → phát → ngủ
    │       ├── buckle_sensor.cpp    #   đọc công tắc từ
    │       ├── ble_beacon.cpp       #   phát gói quảng bá (thay ble_link.cpp)
    │       └── sleep_manager.cpp    #   tính lần thức tiếp theo, đặt chân đánh thức, ngủ sâu
    └── pi/                          # SẼ VIẾT LẠI: các file ở mục 6.2
```

Code cũ trong `helmet/firmware/` và `helmet/pi/` (kiểu kết nối GATT) sẽ được **thay thế** ở bước 2 và bước 4.

---

## 8. Thứ tự làm

| # | Việc | Ai | Xong khi |
|---|---|---|---|
| 1 | Chạy `helmet/ble_test` (đã có) với lưỡi gà + nRF Connect | Minh | Đưa nam châm vào/ra, điện thoại thấy đổi đúng |
| 2 | Viết firmware mới: ngủ sâu + quảng bá + 3 trạng thái | Claude | ✅ 25/11 |
| 2b | Nạp firmware mới, xem gói quảng bá trên nRF Connect (tab Scanner) | Minh | Cài quai → thấy gói dày 10 s; tháo → có ngay gói "chưa cài" rồi im |
| 3 | Gỡ đèn nguồn trên mạch ESP32, đo dòng lúc ngủ bằng đồng hồ (thang µA) | Minh | Có số đo → Claude tính lại mục 5 |
| 4 | Viết `ble_scanner.py`: quét và in trạng thái mũ ra terminal | Claude | ✅ 25/11 |
| 4b | Chạy trên Pi (`pip install bleak`) | Minh | Terminal in đúng đã cài / chưa cài |
| 5 | Viết `helmet_monitor.py` + `buzzer.py`: 3 trạng thái + còi | Claude | ✅ 25/11 |
| 5b | Đấu còi vào Pi, báo Claude chân GPIO + loại còi | Minh | Test đủ 3 tình huống: không đội mũ, tháo quai, tắt nguồn mũ |
| 6 | Viết `display.py` cho màn ST7789 | Claude | ✅ 25/11 |
| 7 | Cài `helmet-monitor.service`, test thật trên xe: vặn chìa → đo thời gian Pi khởi động | Minh + Claude | Vặn chìa không đội mũ → còi kêu |
| 8 | (Tuỳ chọn) Đo % pin mũ, hiện pin yếu lên màn | Claude | — |

---

## 9. Rủi ro đã biết (Minh đã chọn chấp nhận)

**Cài quai rồi chờ hơn 2 phút mới mở chìa** → mũ đã chuyển sang 5 phút/lần → Pi chờ 15 giây không thấy → **còi kêu oan**, lâu nhất tới 5 phút.

Cách xử lý khi gặp: **tháo quai ra cài lại** → mũ gửi ngay và quay lại chế độ 10 giây/lần → còi tắt.

Nếu thấy hay bị thì đổi sang "mỗi 10 giây suốt lúc cài quai": chỉ tốn thêm khoảng 1–2 mAh/ngày, pin vẫn dùng được nhiều tháng.

---

## 10. Còn phải chốt

1. **Module còi**: còi chủ động hay thụ động? Kêu khi chân ở mức cao hay thấp? Cắm chân nào của Pi?
   Code hiện viết cho **còi chủ động, kêu khi mức cao**. Còi thụ động (cần phát tần số) thì báo Claude sửa.
2. **Số line GPIO** trên Orange Pi 4 Pro cho còi, chân DC và RST của màn (xem `helmet/README.md`).
