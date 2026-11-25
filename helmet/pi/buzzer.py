#!/usr/bin/env python3
"""Còi cảnh báo: bíp ngắt quãng (0.5 giây kêu / 0.5 giây nghỉ) khi đang báo động.

Thử còi riêng:  python3 buzzer.py   (kêu 3 tiếng)
"""

import asyncio
import logging

import config

log = logging.getLogger("buzzer")


class Buzzer:
    def __init__(self):
        self.alarm = False  # True = đang báo động, còi bíp liên tục
        self._line = None
        if config.BUZZER_LINE is None:
            log.warning("Chưa đặt BUZZER_LINE trong config.py: còi chỉ in ra terminal")
        else:
            from gpio_line import OutputLine

            self._line = OutputLine(config.GPIO_CHIP, config.BUZZER_LINE, self._level(False))

    @staticmethod
    def _level(on):
        """Đổi "kêu / im" ra mức điện của chân, tuỳ loại còi."""
        return 1 if on == config.BUZZER_ACTIVE_HIGH else 0

    def _write(self, on):
        if self._line is not None:
            self._line.set(self._level(on))

    def set_alarm(self, on):
        if on != self.alarm:
            log.info("Còi: %s", "BẬT" if on else "TẮT")
        self.alarm = on

    async def run(self):
        """Chạy mãi ở nền: đang báo động thì bíp, không thì im."""
        while True:
            if self.alarm:
                self._write(True)
                await asyncio.sleep(config.BEEP_ON_S)
                self._write(False)
                await asyncio.sleep(config.BEEP_OFF_S)
            else:
                self._write(False)
                await asyncio.sleep(0.1)

    def close(self):
        self._write(False)  # Luôn tắt còi trước khi thoát
        if self._line is not None:
            self._line.close()


if __name__ == "__main__":
    import time

    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    b = Buzzer()
    for _ in range(3):
        print("BÍP")
        b._write(True)
        time.sleep(config.BEEP_ON_S)
        b._write(False)
        time.sleep(config.BEEP_OFF_S)
    b.close()
