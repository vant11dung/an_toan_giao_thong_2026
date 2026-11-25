# Hợp đồng giao tiếp: Mũ bảo hiểm (BLE)

Trạng thái: **BẢN NHÁP — chờ nhóm duyệt** · Người phụ trách: Minh · Phiên bản gói: **2**

Tài liệu này là chỗ duy nhất các phần khác cần đọc để làm việc với mũ. Đổi gì ở đây thì báo cả nhóm.

## 1. BLE (mũ → Pi)

- Mũ **chỉ phát quảng bá**, không cho kết nối (ADV_NONCONN_IND). Pi quét liên tục.
- Tên trong gói quảng bá: `ATGT-HELMET-<số>`, ví dụ `ATGT-HELMET-01` (chỉ để nhìn, Pi lọc theo MAC).
- Mỗi lần thức dậy mũ phát ~0.3 giây (~10 gói, cách nhau 30 ms).

### Gói trạng thái: Manufacturer Specific Data, 8 byte

| Byte | Tên | Ý nghĩa |
|---|---|---|
| 0–1 | company ID | `0xFFFF` (little-endian: `FF FF`) |
| 2–3 | magic | `'A' 'T'` (`41 54`) |
| 4 | version | = 2 |
| 5 | flags | bit0 = đã cài quai (1) / chưa (0); các bit khác = 0 |
| 6 | battery | Pin % (0–100), 255 = chưa đo |
| 7 | seq | Tăng 1 mỗi lần thức, quay vòng 0–255 |

### Khi nào mũ gửi

| Tình huống | Mũ làm gì |
|---|---|
| Vừa cài quai | Gửi ngay, sau đó 10 giây/lần trong 2 phút |
| Đang cài (sau 2 phút) | 5 phút/lần |
| Tháo quai | Gửi ngay 1 lần "chưa cài", rồi im hẳn |

### Pi xử lý

- Chưa nhận gói nào sau **15 giây** từ lúc chạy → cảnh báo.
- Gói "chưa cài" → cảnh báo.
- Đang cài mà quá **5 phút 30 giây** không nhận gói → cảnh báo mất tín hiệu.

## 2. Pi: helmet_monitor → chương trình chính (tuỳ chọn)

**Đề xuất, chờ nhóm thống nhất:** đặt `UDP_TARGET = ("127.0.0.1", 5005)` trong `helmet/pi/config.py`.
Mỗi lần trạng thái đổi, gửi một datagram JSON (UTF-8):

```json
{"online": true, "buckled": true, "battery": null}
```

| Trường | Kiểu | Ý nghĩa |
|---|---|---|
| online | bool | Biết được trạng thái mũ (không phải đang chờ / mất tín hiệu) |
| buckled | bool | Đã cài quai. Luôn `false` khi `online` = `false` |
| battery | int / null | Pin %, `null` nếu chưa đo hoặc mất mũ |

Chương trình chính chỉ cần đọc `buckled`.

## 3. Còn phải chốt

1. Chưa cài mũ thì xe chỉ cảnh báo hay chặn không cho chạy.
2. Cách chương trình chính nhận trạng thái (UDP như trên, hay cách khác).
