# an_toan_giao_thong_2026

## Sơ đồ hệ thống

```
[Mũ: ESP32-C3] ~~BLE~~> [Orange Pi trên xe] <- camera/NPU (vùng mù)
                                            <- GPS
```

## Các phần

Mỗi người một thư mục riêng. Chỗ giao nhau giữa các phần viết trong `docs/giao_tiep/`.

| Phần | Người làm | Thư mục | Hợp đồng giao tiếp |
|---|---|---|---|
| Mũ bảo hiểm (BLE) | Minh | [helmet/](helmet/) | [docs/giao_tiep/mu_bao_hiem_ble.md](docs/giao_tiep/mu_bao_hiem_ble.md) |
| Camera / NPU | Thăng | `MAIN/` (chưa chuyển vào repo) | |
| GPS | ? | `tools/gps.py` (chưa chuyển vào repo) | |
