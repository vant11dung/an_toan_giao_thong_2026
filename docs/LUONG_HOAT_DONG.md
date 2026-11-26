# Luồng hoạt động: Cảnh báo cài quai mũ bảo hiểm

Cập nhật: 26/11/2026 · Viết cho: Minh · Đọc file này trước, rồi mới đọc [KE_HOACH_MU_BAO_HIEM.md](KE_HOACH_MU_BAO_HIEM.md) nếu cần chi tiết (tính pin, lý do chọn).

---

## 1. Hệ thống làm gì

**Mở chìa khoá xe mà chưa cài quai mũ → xe kêu còi và hiện cảnh báo lên màn hình.**

Có đúng **2 thiết bị**, nói chuyện với nhau qua **Bluetooth (BLE)**:

```
     TRÊN MŨ                                          TRÊN XE
┌──────────────────────┐                         ┌──────────────────────────┐
│ Công tắc lưỡi gà     │                         │ Orange Pi 4 Pro          │
│ (ở khoá quai)        │                         │ (lấy điện từ xe)         │
│        │             │                         │        │                 │
│        ▼             │   "Tôi đã cài quai!"    │        ▼                 │
│ ESP32-C3 Super Mini  │ ~~~~~~~~~~~~~~~~~~~~~~> │ Chương trình Python      │
│ (chạy pin 500 mAh)   │   BLE quảng bá          │        │                 │
│                      │   (hét lên, không cần   │   ┌────┴─────┐           │
│                      │    kết nối)             │   ▼          ▼           │
└──────────────────────┘                         │  Còi      Màn ST7789     │
                                                 └──────────────────────────┘
```

---

## 2. Code nào chạy ở đâu

Toàn bộ code nằm trong `an_toan_giao_thong_2026/helmet/`. Có **3 thư mục**, đừng nhầm:

| Thư mục | Chạy trên | Dùng để làm gì | Còn dùng không? |
|---|---|---|---|
| `helmet/firmware/` | **ESP32 trên mũ** | **Code thật của mũ**: đọc quai, phát BLE, ngủ sâu | ✅ **Code chính** |
| `helmet/pi/` | **Orange Pi trên xe** | **Code thật của xe**: nghe BLE, kêu còi, hiện màn | ✅ **Code chính** |
| `helmet/ble_test/` | ESP32 | Bản thử cũ, chỉ để kiểm tra lưỡi gà với điện thoại. Kiểu BLE khác, **Pi không đọc được** | ⚠️ Chỉ để thử phần cứng |

**Nhớ:** nạp ESP32 thì dùng `helmet/firmware/`. Copy sang Pi thì dùng `helmet/pi/`.

---

## 3. Chuyện gì xảy ra khi đi xe (kể theo thời gian)

| # | Bạn làm gì | Mũ (ESP32) làm gì | Xe (Pi) làm gì | Màn hình | Còi |
|---|---|---|---|---|---|
| 1 | Mũ để trên kệ, quai mở | **Ngủ sâu**, không phát gì, gần như không tốn pin | (xe tắt) | — | — |
| 2 | **Cài quai** | Lưỡi gà đóng → chip **thức dậy ngay** → phát "đã cài" 0.3 giây → ngủ 10 giây → lặp lại | (xe tắt) | — | — |
| 3 | **Mở chìa** | Vẫn phát "đã cài" mỗi 10 giây | Pi khởi động (~20–40 giây), chương trình chạy, **chờ tối đa 15 giây** | `ĐANG TÌM MŨ – còn 15 giây` | im |
| 4 | — | — | Nghe thấy "đã cài" | `ĐÃ CÀI QUAI MŨ` (nền xanh) | im |
| 5 | Đi xe | Sau 2 phút kể từ lúc cài: chuyển sang phát **5 phút/lần** | Mỗi gói nhận được là "mũ vẫn ổn" | `ĐÃ CÀI QUAI MŨ` | im |
| 6 | **Tháo quai giữa đường** | Lưỡi gà hở → chip **thức dậy ngay** → phát "chưa cài" → **ngủ hẳn** | Nghe thấy "chưa cài" | `CHƯA CÀI QUAI MŨ! – Hãy cài quai` (nền đỏ) | **bíp bíp** |
| 7 | Cài lại quai | Như bước 2 | Nghe "đã cài" | `ĐÃ CÀI QUAI MŨ` | tắt |
| 8 | Tắt xe | Vẫn phát 5 phút/lần tới khi tháo quai | Tắt nguồn | — | — |

### Các tình huống báo động

| Tình huống | Pi phát hiện thế nào | Màn hình |
|---|---|---|
| Mở chìa mà **không đội / không cài mũ** | 15 giây không nghe thấy gì | `CHƯA CÀI QUAI MŨ! – Không thấy mũ` (đỏ) |
| **Tháo quai** khi đang đi | Nhận gói "chưa cài" | `CHƯA CÀI QUAI MŨ! – Hãy cài quai` (đỏ) |
| **Mũ hết pin / ra xa xe** | Đang cài mà **5 phút 30 giây** không nghe gì | `MẤT TÍN HIỆU MŨ! – Kiểm tra pin mũ` (cam) |

Còi: **0.5 giây kêu / 0.5 giây nghỉ**, kêu tới khi nghe được "đã cài".

---

## 4. Dữ liệu chạy từ quai tới còi như thế nào

```
 Lưỡi gà ──> GPIO 4 ──> ESP32 đọc ──> đóng gói 8 byte ──> phát BLE
 (đóng/hở)   (LOW/HIGH)  (lọc dội)    "đã cài/chưa cài"   (0.3 giây)
                                                              │
                                                              ▼  không khí
 Còi + màn <── helmet_monitor.py <── ble_scanner.py <── Bluetooth của Pi
              (quyết định:          (lọc đúng mũ,
               OK hay báo động)      giải mã 8 byte)
```

### Gói tin 8 byte (mũ gửi cho Pi)

| Byte | Nội dung | Ví dụ |
|---|---|---|
| 0–1 | Mã `FF FF` | `FF FF` |
| 2–3 | Chữ `A` `T` (để Pi biết đúng là mũ ATGT) | `41 54` |
| 4 | Phiên bản gói = 2 | `02` |
| 5 | **Quai: 1 = đã cài, 0 = chưa cài** | `01` |
| 6 | % pin (255 = chưa đo) | `FF` |
| 7 | Số thứ tự, tăng mỗi lần gửi | `0C` |

Trên app nRF Connect, phần Manufacturer data sẽ hiện `41 54 02 01 FF 0C`. Hai byte `FF FF` đầu được app tách riêng ra.

Chi tiết chính thức: [giao_tiep/mu_bao_hiem_ble.md](giao_tiep/mu_bao_hiem_ble.md).

---

## 5. Bên trong ESP32: file nào làm gì

Thư mục `helmet/firmware/`:

| File | Nhiệm vụ |
|---|---|
| `include/config.h` | **Mọi con số chỉnh được**: chân GPIO 4, 10 giây, 2 phút, 5 phút, 0.3 giây, `TEST_MODE`, `DEBUG_SERIAL` |
| `include/helmet_protocol.h` | Khuôn gói 8 byte ở mục 4 |
| `src/main.cpp` | **Điều phối**: mỗi lần thức dậy làm 4 việc (dưới đây) |
| `src/buckle_sensor.cpp` | Đọc lưỡi gà, chờ giá trị đứng yên 50 ms mới tin (lọc dội) |
| `src/ble_beacon.cpp` | Bật BLE, phát gói 0.3 giây rồi dừng |
| `src/sleep_manager.cpp` | Hẹn giờ thức dậy + hẹn "thức khi quai đổi", rồi cho chip ngủ sâu |

### Mỗi lần ESP32 thức dậy (`main.cpp`)

```
thức dậy (do: bật nguồn / cài-tháo quai / hết giờ hẹn)
   │
   ├─ 1. Đọc quai ................ buckle_sensor.cpp
   ├─ 2. Phát BLE 0.3 giây ....... ble_beacon.cpp
   ├─ 3. Tính lần thức tiếp theo:
   │       chưa cài       → không hẹn giờ (chỉ thức khi cài quai)
   │       vừa cài <2 phút → 10 giây
   │       đã cài >2 phút  → 5 phút
   └─ 4. Ngủ sâu ................. sleep_manager.cpp
```

Ngủ sâu thì chip tắt gần hết. Thức dậy giống như bấm reset: chạy lại từ đầu. Chỉ vài biến đánh dấu `RTC_DATA_ATTR` là còn nhớ, ví dụ "đã cài được bao lâu".

### Hai công tắc trong `config.h`

| Công tắc | = 1 | = 0 |
|---|---|---|
| `TEST_MODE` | **Chế độ thử**: không ngủ, phát BLE **liên tục**. Điện thoại và Pi lúc nào cũng thấy | **Chạy thật**: ngủ sâu, phát 0.3 giây mỗi lần thức |
| `DEBUG_SERIAL` | In chữ ra Serial để xem trên laptop | Không in (đỡ tốn pin) |

**Hiện đang để `TEST_MODE 1`.** Trước khi lắp lên mũ dùng thật phải đổi về `0`.

### Đấu dây mũ

```
   3V3 ──── điện trở 10 kΩ ────┐
                               ├──── GPIO 4
   GND ──── lưỡi gà ───────────┘

   Pin LiPo ── TP4056 (B+/B−) ── OUT+ → 5V,  OUT− → GND  (qua jack 2.54)
```

- **Điện trở 10 kΩ là bắt buộc**: không có nó thì lúc ngủ sâu, tháo quai chip không thức dậy.
- **Rút jack pin ra trước khi cắm USB nạp code.**

---

## 6. Bên trong Pi: file nào làm gì

Thư mục `helmet/pi/`:

| File | Nhiệm vụ |
|---|---|
| `config.py` | **Mọi thứ cần sửa**: MAC của mũ, chân còi, bật/tắt màn, 15 giây, 5 phút 30 giây |
| `ble_scanner.py` | **Tai**: quét Bluetooth liên tục, lọc đúng mũ mình, giải mã 8 byte. Chạy riêng được để thử |
| `helmet_monitor.py` | **Bộ não**: quyết định OK hay báo động, ra lệnh cho còi và màn |
| `buzzer.py` | **Miệng**: bật/tắt còi qua GPIO. Chưa đấu còi thì chỉ in chữ "Còi: BẬT" ra terminal |
| `display.py` | **Mặt**: vẽ chữ tiếng Việt lên màn ST7789. Chưa bật màn thì chỉ in ra terminal |
| `gpio_line.py` | Dụng cụ bật/tắt chân GPIO, buzzer và display dùng chung |
| `helmet-monitor.service` | Cho chương trình **tự chạy khi Pi bật** (tức là khi mở chìa) |

### 5 trạng thái của bộ não (`helmet_monitor.py`)

```
                 ┌──────────────┐
  chương trình   │   WAITING    │  "ĐANG TÌM MŨ – còn N giây"
  vừa chạy ────> │ chờ ≤ 15 giây│
                 └──────┬───────┘
          nghe "đã cài" │        │ hết 15 giây, không nghe gì
                        ▼        ▼
                 ┌──────────┐  ┌────────────┐
                 │    OK    │  │ NO_HELMET  │ còi + "CHƯA CÀI – Không thấy mũ"
                 └──────────┘  └────────────┘
                   │      │
  nghe "chưa cài"  │      │  5 phút 30 giây không nghe gì
                   ▼      ▼
          ┌───────────┐  ┌──────────┐
          │ UNBUCKLED │  │   LOST   │ còi + "MẤT TÍN HIỆU MŨ"
          └───────────┘  └──────────┘
           còi + "CHƯA CÀI – Hãy cài quai"

  Từ bất kỳ trạng thái báo động nào: nghe "đã cài" → về OK, tắt còi.
```

---

## 7. Đã làm gì tới giờ (để hiểu vì sao code như bây giờ)

| Ngày | Việc | Ghi chú |
|---|---|---|
| 24/11 | Bản 1: mũ **giữ kết nối** BLE liên tục với Pi | Đã bỏ |
| 24/11 | Tạo `ble_test/` để thử lưỡi gà + BLE với điện thoại | Vẫn còn, chỉ để thử |
| 24/11 | Minh yêu cầu **ngủ sâu** cho đỡ tốn pin → viết lại kế hoạch | Ngủ sâu thì tắt hẳn Bluetooth, không giữ kết nối được → đổi sang **phát quảng bá** |
| 25/11 | Viết firmware mới (ngủ sâu + quảng bá) và toàn bộ code Pi (còi, màn ST7789) | Commit `376073b` trên nhánh `mu-bao-hiem-quang-ba` |
| sau 25/11 | Thêm `TEST_MODE` vào firmware | **Chưa commit** |

---

## 8. Hiện trạng

| Phần | Code | Đã test trên phần cứng? |
|---|---|---|
| Firmware mũ | ✅ Có | ❌ Chưa. Điện thoại chưa thấy BLE (đang tìm hiểu) |
| Pi: quét BLE | ✅ Có | ❌ Chưa chạy trên Pi |
| Pi: bộ não | ✅ Có | ❌ |
| Pi: còi | ✅ Có | ❌ Chưa đấu, chưa biết chân |
| Pi: màn ST7789 | ✅ Có | ❌ Chưa đấu |
| Tự chạy khi mở chìa | ✅ Có | ❌ |

Code **chỉ nằm trên laptop**: nhánh `mu-bao-hiem-quang-ba`, **chưa push** lên GitHub.

---

## 9. Làm gì tiếp, theo thứ tự

Bước trước xong mới sang bước sau. Hỏng ở bước nào thì biết lỗi nằm ở đó.

| # | Việc | Ai | Xong khi |
|---|---|---|---|
| 1 | Đấu lưỡi gà + điện trở 10 kΩ vào ESP32 (mục 5) | Minh | — |
| 2 | Nạp `helmet/firmware/` với **`TEST_MODE 1`** | Minh | Serial in `[thu] quai: DA CAI / CHUA CAI` đổi theo nam châm |
| 3 | Mở nRF Connect → Scanner → tìm `ATGT-HELMET-01` | Minh | Thấy `41 54 02 01 ...` khi có nam châm, `... 00 ...` khi không |
| 4 | Copy `helmet/pi/` sang Pi, `sudo pip3 install bleak` | Minh | — |
| 5 | Trên Pi: `sudo python3 ble_scanner.py` | Minh | In `Mũ AA:BB:...: ĐÃ CÀI` / `CHƯA CÀI` theo nam châm |
| 6 | Chép MAC vào `HELMET_MAC` trong `config.py` | Minh | — |
| 7 | Trên Pi: `sudo python3 helmet_monitor.py` (chưa cần còi/màn) | Minh | Terminal in đúng 3 tình huống ở mục 3 |
| 8 | Đổi **`TEST_MODE 0`**, nạp lại, test lại bước 7 với chế độ ngủ sâu | Minh | Tháo quai → Pi báo ngay |
| 9 | Đấu còi, báo Claude chân GPIO → sửa `config.py` | Minh + Claude | `python3 buzzer.py` kêu 3 tiếng |
| 10 | Đấu màn ST7789, bật SPI → sửa `config.py` | Minh + Claude | `python3 display.py` hiện chữ |
| 11 | Cài `helmet-monitor.service`, thử trên xe thật | Minh | Mở chìa không đội mũ → còi kêu |
| 12 | Gỡ đèn nguồn trên ESP32, đo dòng ngủ | Minh | Có số đo → Claude tính lại thời gian pin |

**Đang ở bước 2–3.**

---

## 10. Còn phải chốt

1. **Còi**: loại chủ động hay thụ động? Cắm chân nào của Pi?
2. **Chân DC, RST của màn** cắm chân GPIO nào của Pi?
3. **Đề xuất của Claude:** tháo quai thì gửi "chưa cài" thêm 2 lần (cách 10 giây) rồi mới ngủ hẳn. Bây giờ mới gửi 1 lần, Pi lỡ mất thì phải 5 phút 30 giây sau mới báo. → **Minh chưa trả lời.**
4. Bỏ hay giữ khối `TEST_MODE` trong code?

---

## 11. Từ điển nhanh

| Từ | Nghĩa |
|---|---|
| **BLE** | Bluetooth tiết kiệm năng lượng, loại trong vòng đeo tay, tai nghe |
| **Phát quảng bá** (advertising) | Thiết bị "hét" dữ liệu ra xung quanh, ai quét thì nghe được, không cần kết nối |
| **Ngủ sâu** (deep sleep) | ESP32 tắt gần hết, chỉ ăn vài µA. Thức dậy là chạy lại từ đầu |
| **GPIO** | Chân điều khiển trên chip, đọc hoặc xuất mức điện HIGH (3.3 V) / LOW (0 V) |
| **Điện trở kéo lên** (pull-up) | Điện trở nối chân GPIO lên 3.3 V, để chân ở HIGH khi công tắc hở |
| **Lọc dội** (debounce) | Công tắc khi đóng/mở bị nảy vài ms, phải chờ đứng yên mới tin |
| **MAC** | "Số chứng minh" của thiết bị Bluetooth, dạng `AA:BB:CC:DD:EE:FF` |
| **seq** | Số thứ tự gói, tăng mỗi lần gửi. Pi dựa vào nó để biết đây là gói mới |
| **nRF Connect** | App điện thoại để xem các thiết bị BLE xung quanh |
| **Serial Monitor** | Cửa sổ trên laptop xem ESP32 in chữ ra qua cáp USB |
