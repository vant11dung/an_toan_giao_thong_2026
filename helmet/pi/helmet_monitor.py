#!/usr/bin/env python3
"""Bộ não trên Orange Pi: nghe mũ qua BLE, quyết định khi nào kêu còi và hiện gì lên màn.

Chạy:  python3 helmet_monitor.py
Tự chạy khi mở chìa: xem helmet-monitor.service

Các trạng thái (docs/KE_HOACH_MU_BAO_HIEM.md mục 6.1):
    WAITING   : vừa chạy, chờ mũ tối đa 15 giây
    OK        : mũ báo đã cài quai
    NO_HELMET : hết 15 giây đầu mà không thấy mũ       -> còi + "CHƯA CÀI QUAI MŨ!"
    UNBUCKLED : mũ báo chưa cài / vừa tháo quai         -> còi + "CHƯA CÀI QUAI MŨ!"
    LOST      : đang cài mà quá 5 phút 30 giây im lặng  -> còi + "MẤT TÍN HIỆU MŨ!"
"""

import asyncio
import json
import logging
import signal
import socket
import time

import config
from ble_scanner import HelmetScanner
from buzzer import Buzzer
from display import Display

log = logging.getLogger("monitor")

WAITING, OK, NO_HELMET, UNBUCKLED, LOST = "WAITING", "OK", "NO_HELMET", "UNBUCKLED", "LOST"
ALARM_STATES = {NO_HELMET, UNBUCKLED, LOST}

WHITE, BLACK = (255, 255, 255), (0, 0, 0)
YELLOW, GREEN, RED, ORANGE = (255, 210, 0), (0, 150, 60), (210, 0, 0), (230, 110, 0)

# Trạng thái -> (chữ to, chữ nhỏ, màu chữ, màu nền). {left} = số giây chờ còn lại.
SCREENS = {
    WAITING:   ("ĐANG TÌM MŨ", "còn {left} giây", YELLOW, BLACK),
    OK:        ("ĐÃ CÀI\nQUAI MŨ", "Chúc đi an toàn", WHITE, GREEN),
    NO_HELMET: ("CHƯA CÀI\nQUAI MŨ!", "Không thấy mũ", WHITE, RED),
    UNBUCKLED: ("CHƯA CÀI\nQUAI MŨ!", "Hãy cài quai", WHITE, RED),
    LOST:      ("MẤT TÍN\nHIỆU MŨ!", "Kiểm tra pin mũ", WHITE, ORANGE),
}


def decide(now, started_at, last):
    """Tính trạng thái từ gói gần nhất. last = (buckled, seen_at) hoặc None nếu chưa nhận gói nào."""
    if last is None:
        return WAITING if now - started_at < config.WAIT_FIRST_S else NO_HELMET
    buckled, seen_at = last
    if not buckled:
        return UNBUCKLED  # Mũ chưa cài thì ngủ hẳn, im lặng là bình thường -> không tính mất tín hiệu
    if now - seen_at > config.LOST_AFTER_S:
        return LOST
    return OK


async def run():
    started_at = time.monotonic()
    last = None      # (buckled, seen_at) của gói gần nhất
    battery = None

    def on_packet(pkt):
        nonlocal last, battery
        last = (pkt.buckled, time.monotonic())
        battery = pkt.battery

    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) if config.UDP_TARGET else None
    display = Display()
    buzzer = Buzzer()
    scanner = HelmetScanner(config.HELMET_MAC, on_packet)

    # Tắt gọn gàng (tắt còi, xoá màn) khi bấm Ctrl+C hoặc systemd dừng chương trình
    stop = asyncio.Event()
    loop = asyncio.get_running_loop()
    for sig in (signal.SIGINT, signal.SIGTERM):
        loop.add_signal_handler(sig, stop.set)

    buzzer_task = asyncio.create_task(buzzer.run())
    await scanner.start()
    log.info("Đã bắt đầu, chờ mũ tối đa %d giây", config.WAIT_FIRST_S)

    state, shown = None, None
    try:
        while not stop.is_set():
            now = time.monotonic()
            new_state = decide(now, started_at, last)

            if new_state != state:
                log.info("Trạng thái: %s -> %s", state, new_state)
                state = new_state
                buzzer.set_alarm(state in ALARM_STATES)
                if udp:
                    online = state in (OK, UNBUCKLED)
                    msg = {"online": online, "buckled": state == OK, "battery": battery if online else None}
                    udp.sendto(json.dumps(msg).encode(), config.UDP_TARGET)

            # Vẽ lại màn khi trạng thái đổi, hoặc số giây đếm ngược đổi
            left = max(0, int(config.WAIT_FIRST_S - (now - started_at) + 0.999))
            title, subtitle, fg, bg = SCREENS[state]
            subtitle = subtitle.format(left=left)
            if (title, subtitle) != shown:
                shown = (title, subtitle)
                display.show(title, subtitle, fg, bg)

            try:
                await asyncio.wait_for(stop.wait(), timeout=0.2)
            except asyncio.TimeoutError:
                pass
    finally:
        log.info("Dừng chương trình")
        buzzer_task.cancel()
        buzzer.close()
        display.close()
        await scanner.stop()


def main():
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(name)s: %(message)s")
    asyncio.run(run())


if __name__ == "__main__":
    main()
