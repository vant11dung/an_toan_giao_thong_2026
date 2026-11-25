// =====================================================================
// ble_beacon.cpp — phát gói QUẢNG BÁ BLE (không kết nối).
//
// Mũ giống cái loa: hét vài lần "tôi đã cài quai!" rồi im. Ai đang quét
// (Pi, điện thoại) thì nghe thấy. Không có ai "bắt máy", không kết nối.
//
// Một gói quảng bá tối đa 31 byte, gồm:
//   - flags (3 byte): báo đây là thiết bị BLE thường
//   - manufacturer data (2 + 8 byte): gói trạng thái HelmetAdvPacket
//   - tên mũ (2 + 14 byte): "ATGT-HELMET-01", để dễ nhận ra trong nRF Connect
//   Tổng 29 byte, vừa đủ.
// =====================================================================

#include "ble_beacon.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "config.h"
#include "helmet_protocol.h"

void beacon_send(bool buckled, uint8_t battery, uint8_t seq) {
    // --- Đóng gói 8 byte ---
    HelmetAdvPacket pkt;
    pkt.company_id = HELMET_COMPANY_ID;
    pkt.magic[0] = 'A';
    pkt.magic[1] = 'T';
    pkt.version = HELMET_PROTOCOL_VERSION;
    pkt.flags = buckled ? HELMET_FLAG_BUCKLED : 0;
    pkt.battery = battery;
    pkt.seq = seq;

    // --- Bật BLE ---
    NimBLEDevice::init("");                    // Tên đặt trong gói quảng bá bên dưới
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);    // Công suất tối đa (+9 dBm) cho sóng xa hơn

    // --- Soạn nội dung gói quảng bá ---
    NimBLEAdvertisementData data;
    data.setFlags(0x06);   // 0x06 = "tìm thấy được" + "không hỗ trợ Bluetooth cổ điển"
    data.setManufacturerData(std::string((const char*)&pkt, sizeof(pkt)));
    data.setName(HELMET_NAME);

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setAdvertisementData(data);
    adv->setScanResponse(false);                         // Không cần gói phụ
    adv->setAdvertisementType(BLE_GAP_CONN_MODE_NON);    // Không cho kết nối, chỉ phát
    adv->setMinInterval(ADV_INTERVAL_UNITS);             // ~30 ms một gói
    adv->setMaxInterval(ADV_INTERVAL_UNITS);

    // --- Phát ~0.3 giây rồi dừng ---
    adv->start();
    delay(ADV_BURST_MS);
    adv->stop();
    // Không cần tắt BLE: ngủ sâu sẽ tắt hẳn mọi thứ.
}
