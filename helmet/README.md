# Mũ bảo hiểm (BLE) — Minh

ESP32-C3 Super Mini trên mũ đọc công tắc quai, phát gói **quảng bá BLE** rồi ngủ sâu.
Orange Pi trên xe quét BLE liên tục, chưa cài quai thì **kêu còi + hiện cảnh báo** trên màn ST7789.

```
[Công tắc quai] -> ESP32-C3 (mũ) ~~BLE quảng bá~~> Orange Pi: helmet_monitor.py -> còi + màn hình
```

Kế hoạch chi tiết: [docs/KE_HOACH_MU_BAO_HIEM.md](../docs/KE_HOACH_MU_BAO_HIEM.md).
Định dạng gói: [docs/giao_tiep/mu_bao_hiem_ble.md](../docs/giao_tiep/mu_bao_hiem_ble.md).

## Thư mục

- `ble_test/`: test BLE riêng với điện thoại (kiểu kết nối cũ, giữ lại để thử lưỡi gà).
- `firmware/`: firmware mũ thật: ngủ sâu + quảng bá.
- `pi/`: chương trình chạy trên Orange Pi.

---

## 1. Mũ (ESP32-C3 Super Mini)

### Đấu dây

| Linh kiện | Nối vào |
|---|---|
| Công tắc từ, chân 1 | GPIO 4 |
| Công tắc từ, chân 2 | GND |
| **Điện trở 10 kΩ** | GPIO 4 ↔ 3V3 (**bắt buộc**, xem `firmware/include/config.h`) |
| TP4056 OUT+ | 5V (qua jack 2.54) |
| TP4056 OUT− | GND (qua jack 2.54) |

Pin LiPo nối vào B+ / B− của TP4056. Hết pin thì rút jack, cắm TP4056 đi sạc.

### Nạp code

> ⚠️ **Rút jack pin ra trước khi cắm USB vào ESP32.**

```bash
cd helmet/firmware
pio run -t upload      # build + nạp
pio device monitor     # xem Serial, 115200
```

**ESP32 đang ngủ sâu thì laptop không thấy cổng USB, nên không nạp được.** Cách nạp:
giữ nút **BOOT** → bấm nhả **RESET** → nhả **BOOT** → chạy lại `pio run -t upload`.

Serial cũng chỉ hiện vài dòng mỗi lần mũ thức dậy rồi mất kết nối. Việc này bình thường.

### Thử với điện thoại (app nRF Connect, tab Scanner)

1. Tìm `ATGT-HELMET-01`, mở phần "Manufacturer data": thấy `0xFFFF` + `41 54 02 ...`.
2. Cài quai (đưa nam châm vào): byte thứ 4 sau `41 54` = `01`, gói mới cứ 10 giây một lần.
3. Tháo quai: có ngay gói với byte đó = `00`, sau đó im hẳn.

Chạy bằng pin thật thì đặt `DEBUG_SERIAL 0` trong `config.h` rồi nạp lại.

---

## 2. Orange Pi

### Cài thư viện (1 lần)

```bash
sudo apt install python3-pip python3-pil python3-numpy python3-libgpiod fonts-dejavu-core gpiod
sudo pip3 install bleak spidev        # lỗi "externally-managed" thì thêm --break-system-packages
```

### Bật SPI cho màn hình

`sudo orangepi-config` → System → Hardware → bật mục có chữ `spi` → khởi động lại.
Kiểm tra: `ls /dev/spidev*` phải thấy ví dụ `/dev/spidev0.0`, ghi số vào `SPI_BUS`, `SPI_DEVICE` trong `pi/config.py`.

### Đấu màn ST7789 và còi

| Chân màn | Orange Pi |
|---|---|
| GND | GND |
| VCC | 3.3V |
| SCL | SPI CLK |
| SDA | SPI MOSI |
| CS | SPI CS0 |
| DC | 1 chân GPIO bất kỳ |
| RST | 1 chân GPIO bất kỳ |
| BL | 3.3V |

Chân SPI CLK / MOSI / CS0 nằm ở đâu trên header 40 chân: xem sơ đồ chân trong manual Orange Pi 4 Pro
(hoặc lệnh `gpio readall` nếu đã cài wiringOP).

Còi: chân `+` (hoặc `I/O`) → 1 chân GPIO, chân `−` → GND, `VCC` (nếu có) → 3.3V hoặc 5V.

### Tìm số "line" của chân GPIO

`config.py` cần **số line**, không phải số chân vật lý (1–40). Cách tìm:

```bash
gpioinfo        # liệt kê các chip và line
```

Chân Allwinner tên dạng `P<chữ><số>`, ví dụ `PC7`: line = (thứ tự chữ, A=0, B=1, C=2...) × 32 + số
→ `PC7` = 2 × 32 + 7 = **71**. Ghi vào `BUZZER_LINE`, `LCD_DC_LINE`, `LCD_RST_LINE`.
Không chắc thì gửi kết quả `gpioinfo` + tên chân cho Claude.

### Thử từng phần

```bash
cd helmet/pi
sudo python3 ble_scanner.py     # cài/tháo quai, thấy dòng "Mũ AA:BB:...: ĐÃ CÀI". Chép MAC vào config.py
sudo python3 buzzer.py          # còi kêu 3 tiếng
sudo python3 display.py         # màn lần lượt hiện 5 màn hình
sudo python3 helmet_monitor.py  # chạy thật
```

### Tự chạy khi mở chìa

Xem hướng dẫn ở đầu file `pi/helmet-monitor.service`.
