#pragma once
#include <stdint.h>

// Gói quảng bá mũ -> Pi, 8 byte, nằm trong phần "Manufacturer Specific Data".
// Đổi định dạng thì tăng HELMET_PROTOCOL_VERSION và cập nhật
// docs/giao_tiep/mu_bao_hiem_ble.md + helmet/pi/ble_scanner.py.

#define HELMET_COMPANY_ID 0xFFFF  // mã dành cho thử nghiệm, không thuộc hãng nào
#define HELMET_PROTOCOL_VERSION 2

#define HELMET_FLAG_BUCKLED 0x01  // bit0: 1 = đã cài quai

#define HELMET_BATTERY_UNKNOWN 255

typedef struct __attribute__((packed)) {
    uint16_t company_id;  // byte 0–1: 0xFFFF
    uint8_t magic[2];     // byte 2–3: 'A' 'T' để Pi biết đúng là mũ ATGT
    uint8_t version;      // byte 4: = HELMET_PROTOCOL_VERSION
    uint8_t flags;        // byte 5: HELMET_FLAG_*
    uint8_t battery;      // byte 6: 0–100 %, 255 = chưa đo
    uint8_t seq;          // byte 7: tăng mỗi lần thức. Linux bỏ qua gói giống hệt gói trước nên bắt buộc phải đổi
} HelmetAdvPacket;

static_assert(sizeof(HelmetAdvPacket) == 8, "HelmetAdvPacket phải đúng 8 byte");
