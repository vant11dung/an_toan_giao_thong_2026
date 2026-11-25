#!/usr/bin/env python3
"""Màn hình ST7789 1.54" 240x240 (SPI), hiện chữ cảnh báo.

Đấu dây (chân trên màn → Orange Pi):
    GND → GND       VCC → 3.3V      SCL → SPI SCLK    SDA → SPI MOSI
    RST → GPIO      DC  → GPIO      CS  → SPI CS0     BL  → 3.3V (đèn nền luôn sáng)

Thử màn riêng:  python3 display.py   (lần lượt hiện các màn hình, mỗi cái 2 giây)

Khi DISPLAY_ENABLED = False trong config.py: chỉ in chữ ra terminal.
"""

import logging
import time

from PIL import Image, ImageDraw, ImageFont

import config

log = logging.getLogger("display")

WIDTH = HEIGHT = 240


class ST7789:
    """Driver tối thiểu cho chip ST7789: khởi động màn và đẩy 1 ảnh lên."""

    def __init__(self):
        import spidev

        from gpio_line import OutputLine

        self.spi = spidev.SpiDev()
        self.spi.open(config.SPI_BUS, config.SPI_DEVICE)
        self.spi.max_speed_hz = config.SPI_SPEED_HZ
        self.spi.mode = config.SPI_MODE
        # DC: 0 = byte đang gửi là LỆNH, 1 = byte đang gửi là DỮ LIỆU
        self.dc = OutputLine(config.GPIO_CHIP, config.LCD_DC_LINE, 0)
        self.rst = None
        if config.LCD_RST_LINE is not None:
            self.rst = OutputLine(config.GPIO_CHIP, config.LCD_RST_LINE, 1)
        self._init_panel()

    def _command(self, cmd, data=None):
        self.dc.set(0)
        self.spi.writebytes([cmd])
        if data:
            self.dc.set(1)
            self.spi.writebytes2(data)

    def _init_panel(self):
        if self.rst is not None:  # Reset cứng: kéo RST xuống rồi lên
            self.rst.set(0)
            time.sleep(0.05)
            self.rst.set(1)
            time.sleep(0.15)
        self._command(0x01)            # Reset mềm
        time.sleep(0.15)
        self._command(0x11)            # Thoát chế độ ngủ
        time.sleep(0.12)
        self._command(0x3A, [0x55])    # Màu 16 bit (RGB565)
        self._command(0x36, [0x00])    # Hướng quét mặc định (xoay hình làm bằng phần mềm)
        self._command(0x21)            # Đảo màu: màn IPS cần lệnh này, không thì màu bị ngược
        self._command(0x13)            # Chế độ hiển thị thường
        self._command(0x29)            # Bật hiển thị
        time.sleep(0.05)

    def show(self, image):
        """Đẩy ảnh PIL 240x240 lên màn."""
        import numpy as np

        if config.LCD_ROTATION:
            image = image.rotate(-config.LCD_ROTATION)
        # Đổi mỗi điểm ảnh từ 3 byte (R, G, B) sang 2 byte RGB565
        a = np.asarray(image.convert("RGB"), dtype=np.uint16)
        rgb565 = ((a[..., 0] & 0xF8) << 8) | ((a[..., 1] & 0xFC) << 3) | (a[..., 2] >> 3)
        self._command(0x2A, [0, 0, 0, WIDTH - 1])    # Cột 0..239
        self._command(0x2B, [0, 0, 0, HEIGHT - 1])   # Hàng 0..239
        self._command(0x2C, rgb565.astype(">u2").tobytes())   # Ghi điểm ảnh

    def close(self):
        self.spi.close()
        self.dc.close()
        if self.rst is not None:
            self.rst.close()


def _load_font(size):
    try:
        return ImageFont.truetype(config.FONT_PATH, size)
    except OSError:
        log.warning("Không thấy font %s, dùng font mặc định (có thể lỗi dấu tiếng Việt)", config.FONT_PATH)
        try:
            return ImageFont.load_default(size)  # Pillow >= 10.1
        except TypeError:
            return ImageFont.load_default()


def _fit_font(draw, text, max_w, max_h, start=52):
    """Chọn cỡ chữ to nhất mà vẫn vừa khung."""
    for size in range(start, 12, -2):
        font = _load_font(size)
        l, t, r, b = draw.multiline_textbbox((0, 0), text, font=font, align="center", spacing=6)
        if r - l <= max_w and b - t <= max_h:
            return font
    return _load_font(12)


def render(title, subtitle, fg, bg):
    """Vẽ màn hình: tiêu đề to ở giữa, dòng phụ nhỏ ở dưới."""
    img = Image.new("RGB", (WIDTH, HEIGHT), bg)
    draw = ImageDraw.Draw(img)
    title_font = _fit_font(draw, title, WIDTH - 16, 150)
    draw.multiline_text((WIDTH // 2, 100), title, font=title_font, fill=fg, anchor="mm", align="center", spacing=6)
    if subtitle:
        sub_font = _fit_font(draw, subtitle, WIDTH - 16, 30, start=24)
        draw.text((WIDTH // 2, 205), subtitle, font=sub_font, fill=fg, anchor="mm")
    return img


class Display:
    def __init__(self):
        self._lcd = None
        if config.DISPLAY_ENABLED:
            self._lcd = ST7789()
        else:
            log.warning("DISPLAY_ENABLED = False: màn hình chỉ in ra terminal")

    def show(self, title, subtitle, fg, bg):
        log.info("Màn hình: %s | %s", title.replace("\n", " "), subtitle)
        if self._lcd is not None:
            self._lcd.show(render(title, subtitle, fg, bg))

    def close(self):
        if self._lcd is not None:
            self._lcd.show(Image.new("RGB", (WIDTH, HEIGHT), "black"))
            self._lcd.close()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    from helmet_monitor import SCREENS

    d = Display()
    try:
        for state, (title, subtitle, fg, bg) in SCREENS.items():
            d.show(title, subtitle.format(left=15), fg, bg)
            time.sleep(2)
    finally:
        d.close()
