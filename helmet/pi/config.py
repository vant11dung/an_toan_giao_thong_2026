"""Cấu hình chương trình mũ trên Orange Pi. Mọi thứ cần sửa đều nằm ở file này."""

# ---------- Mũ ----------
# MAC của mũ mình. Để None thì nhận mọi mũ ATGT (chỉ dùng lúc thử).
# Chạy `python3 ble_scanner.py` sẽ in MAC ra, chép vào đây, ví dụ "AA:BB:CC:DD:EE:FF".
HELMET_MAC = None

# ---------- Mốc thời gian (giây) ----------
WAIT_FIRST_S = 15      # Chương trình chạy xong mà 15 giây chưa thấy mũ -> cảnh báo
LOST_AFTER_S = 330     # Đang cài mà 5 phút 30 giây không nhận gói nào -> mất tín hiệu

# ---------- GPIO ----------
# Orange Pi đánh số chân GPIO theo "chip" + "line". Cách tìm số: xem helmet/README.md mục Pi.
GPIO_CHIP = "/dev/gpiochip0"

# ---------- Còi ----------
BUZZER_LINE = None          # Số line của chân còi. None = chưa đấu, chỉ in ra terminal
BUZZER_ACTIVE_HIGH = True   # True: chân lên mức cao thì còi kêu. Còi kêu ngược thì đổi False
BEEP_ON_S = 0.5             # Kêu 0.5 giây
BEEP_OFF_S = 0.5            # Nghỉ 0.5 giây

# ---------- Màn hình ST7789 240x240 (SPI) ----------
DISPLAY_ENABLED = False     # Đấu xong màn thì đổi True
SPI_BUS = 0                 # Màn ở /dev/spidev<BUS>.<DEVICE>, xem bằng: ls /dev/spidev*
SPI_DEVICE = 0
SPI_SPEED_HZ = 40_000_000   # Màn bị sọc/nhiễu thì giảm xuống 20_000_000
SPI_MODE = 0                # Màn đen thui thì thử 3
LCD_DC_LINE = None          # Chân DC của màn (line GPIO)
LCD_RST_LINE = None         # Chân RST của màn (line GPIO). None nếu nối RST thẳng lên 3.3V
LCD_ROTATION = 0            # Xoay hình: 0, 90, 180, 270
FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"   # Font có tiếng Việt

# ---------- Gửi trạng thái cho chương trình chính (tuỳ chọn) ----------
# Ví dụ ("127.0.0.1", 5005). None = không gửi.
UDP_TARGET = None
