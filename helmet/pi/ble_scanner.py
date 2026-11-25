#!/usr/bin/env python3
"""Quét BLE, tìm gói quảng bá của mũ, giải mã ra "đã cài / chưa cài".

Chạy riêng file này để thử (bước 4b):
    python3 ble_scanner.py
Mỗi khi mũ gửi gói mới, in 1 dòng kèm MAC của mũ. Chép MAC vào config.py.

Định dạng gói: docs/giao_tiep/mu_bao_hiem_ble.md
"""

from __future__ import annotations  # cho phép viết "int | None" trên Python cũ (3.8, 3.9)

import asyncio
import logging
from dataclasses import dataclass

from bleak import BleakScanner

COMPANY_ID = 0xFFFF
MAGIC = b"AT"
PROTOCOL_VERSION = 2
FLAG_BUCKLED = 0x01
BATTERY_UNKNOWN = 255

log = logging.getLogger("scanner")


@dataclass
class HelmetPacket:
    address: str
    buckled: bool
    battery: int | None  # % pin, None = chưa đo
    seq: int
    rssi: int


def parse(address, data, rssi):
    """Giải mã 6 byte sau company ID. Không phải gói của mũ thì trả None.

    bleak đã tách sẵn 2 byte company ID, nên `data` bắt đầu từ byte 'A' 'T'.
    """
    if data is None or len(data) < 6 or data[:2] != MAGIC:
        return None
    version, flags, battery, seq = data[2], data[3], data[4], data[5]
    if version != PROTOCOL_VERSION:
        log.warning("Mũ %s gửi gói phiên bản %d, cần %d. Nạp lại firmware?", address, version, PROTOCOL_VERSION)
        return None
    return HelmetPacket(
        address=address,
        buckled=bool(flags & FLAG_BUCKLED),
        battery=None if battery == BATTERY_UNKNOWN else battery,
        seq=seq,
        rssi=rssi,
    )


class HelmetScanner:
    """Quét liên tục. Mỗi gói hợp lệ của mũ -> gọi on_packet(HelmetPacket)."""

    def __init__(self, mac, on_packet):
        self.mac = mac.upper() if mac else None
        self.on_packet = on_packet
        self._last_seq = {}  # MAC -> seq gần nhất, để chỉ in log khi có gói mới
        self._scanner = BleakScanner(detection_callback=self._on_advertisement)

    def _on_advertisement(self, device, adv):
        pkt = parse(device.address, adv.manufacturer_data.get(COMPANY_ID), adv.rssi)
        if pkt is None:
            return
        if self.mac and device.address.upper() != self.mac:
            return  # Mũ của xe khác
        if self._last_seq.get(pkt.address) != pkt.seq:
            self._last_seq[pkt.address] = pkt.seq
            log.info(
                "Mũ %s: %s, seq=%d, sóng %d dBm",
                pkt.address, "ĐÃ CÀI" if pkt.buckled else "CHƯA CÀI", pkt.seq, pkt.rssi,
            )
        self.on_packet(pkt)

    async def start(self):
        if self.mac is None:
            log.warning("Chưa đặt HELMET_MAC trong config.py: đang nhận MỌI mũ ATGT ở gần")
        await self._scanner.start()

    async def stop(self):
        await self._scanner.stop()


async def _demo():
    import config

    scanner = HelmetScanner(config.HELMET_MAC, lambda pkt: None)
    await scanner.start()
    log.info("Đang quét... cài/tháo quai để thử. Ctrl+C để dừng.")
    try:
        await asyncio.Event().wait()  # chờ mãi
    finally:
        await scanner.stop()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    try:
        asyncio.run(_demo())
    except KeyboardInterrupt:
        pass
