"""Bật/tắt 1 chân GPIO qua libgpiod.

Chạy được với cả 2 bản thư viện gpiod:
- bản 1.x (cài bằng `sudo apt install python3-libgpiod`)
- bản 2.x (cài bằng `pip install gpiod`)
"""

import gpiod


class OutputLine:
    """Một chân GPIO ở chế độ xuất (output)."""

    def __init__(self, chip_path, line, value=0, consumer="atgt-helmet"):
        self.line = line
        if hasattr(gpiod, "request_lines"):  # gpiod bản 2.x
            from gpiod.line import Direction, Value

            self._Value = Value
            self._req = gpiod.request_lines(
                chip_path,
                consumer=consumer,
                config={
                    line: gpiod.LineSettings(
                        direction=Direction.OUTPUT,
                        output_value=Value.ACTIVE if value else Value.INACTIVE,
                    )
                },
            )
            self._line = None
        else:  # gpiod bản 1.x
            self._req = None
            self._line = gpiod.Chip(chip_path).get_line(line)
            self._line.request(consumer=consumer, type=gpiod.LINE_REQ_DIR_OUT, default_vals=[1 if value else 0])

    def set(self, value):
        if self._req is not None:
            self._req.set_value(self.line, self._Value.ACTIVE if value else self._Value.INACTIVE)
        else:
            self._line.set_value(1 if value else 0)

    def close(self):
        if self._req is not None:
            self._req.release()
        else:
            self._line.release()
