// =====================================================================
// ble_test — thử BLE của ESP32-C3 với điện thoại (app nRF Connect).
//
// Đấu công tắc lưỡi gà (reed): 1 chân vào GPIO 4, chân kia vào GND.
// Test được 3 thứ:
//   1. Điện thoại THẤY và KẾT NỐI được ESP32 (tên "ATGT-TEST").
//   2. ESP32 -> điện thoại: mỗi 1 giây NOTIFY một dòng chữ "dem=..., quai=...".
//      Đưa nam châm lại gần / ra xa lưỡi gà thì gửi ngay (giả lập cài/tháo quai).
//   3. Điện thoại -> ESP32: ghi "1" thì bật đèn LED, "0" thì tắt, và in ra Serial.
//
// Gửi dạng CHỮ (không phải byte) để trên app đọc được luôn, không phải dịch hex.
// =====================================================================

#include <Arduino.h>        // Serial, millis, pinMode, digitalRead/Write...
#include <NimBLEDevice.h>   // Thư viện BLE NimBLE

// ---------------- Cấu hình ----------------
#define DEVICE_NAME "ATGT-TEST"   // Tên hiện trong danh sách quét của điện thoại

// UUID riêng cho bản test, khác UUID của firmware mũ thật để khỏi nhầm
#define SERVICE_UUID "f08cbe00-5247-4130-bd80-c2f24294a3b7"
#define TX_CHAR_UUID "f08cbe01-5247-4130-bd80-c2f24294a3b7"   // ESP32 -> điện thoại (READ + NOTIFY)
#define RX_CHAR_UUID "f08cbe02-5247-4130-bd80-c2f24294a3b7"   // điện thoại -> ESP32 (WRITE)

#define REED_PIN 4         // Chân nối công tắc lưỡi gà (chân kia nối GND)
#define REED_ACTIVE_LOW 1  // Có nam châm -> lưỡi gà đóng -> chân về GND -> LOW = "đã cài".
                           // Nếu thấy chạy NGƯỢC (xa nam châm lại báo DA CAI) thì đổi thành 0.
#define REED_DEBOUNCE_MS 200   // Trạng thái phải giữ nguyên 200 ms mới tính (giống firmware thật)

#define LED_PIN 8          // LED xanh trên C3 Super Mini ở GPIO 8, mức LOW = sáng
#define LED_ACTIVE_LOW 1   // Mạch khác mà LED sáng ở mức HIGH thì đổi thành 0

#define NOTIFY_INTERVAL_MS 1000   // Chu kỳ gửi notify (ms)

// ---------------- Biến toàn cục ----------------
static NimBLECharacteristic* tx_char = nullptr;   // Characteristic gửi đi, giữ lại để notify
static volatile bool connected = false;            // Điện thoại có đang kết nối không
static uint32_t counter = 0;                       // Bộ đếm tăng mỗi lần gửi, để thấy dữ liệu đang chạy
static uint32_t last_send = 0;                     // Thời điểm gửi gần nhất (ms)

// Lọc dội lưỡi gà (cùng cách với buckle_sensor.cpp của firmware thật)
static bool reed_stable = false;      // Trạng thái chính thức đã lọc: true = đã cài
static bool reed_last_raw = false;    // Lần đọc thô gần nhất
static uint32_t reed_changed_at = 0;  // Lúc lần đọc thô đổi (ms)

// Đọc thô lưỡi gà: true = có nam châm (đã cài), chưa lọc dội
static bool reed_read_raw() {
    int level = digitalRead(REED_PIN);
#if REED_ACTIVE_LOW
    return level == LOW;
#else
    return level == HIGH;
#endif
}

// Gọi liên tục trong loop(). Trả true đúng 1 lần khi trạng thái ổn định vừa đổi.
static bool reed_update() {
    bool raw = reed_read_raw();
    uint32_t now = millis();
    if (raw != reed_last_raw) {          // Vừa đổi -> có thể đang nảy, reset đồng hồ
        reed_last_raw = raw;
        reed_changed_at = now;
        return false;
    }
    if (raw != reed_stable && now - reed_changed_at >= REED_DEBOUNCE_MS) {
        reed_stable = raw;               // Đứng yên đủ 200 ms -> chấp nhận
        return true;
    }
    return false;
}

// Bật/tắt LED, tự xử lý chuyện LED sáng ở mức LOW hay HIGH
static void set_led(bool on) {
#if LED_ACTIVE_LOW
    digitalWrite(LED_PIN, on ? LOW : HIGH);
#else
    digitalWrite(LED_PIN, on ? HIGH : LOW);
#endif
}

// ---------------------------------------------------------------------
// Sự kiện kết nối / ngắt kết nối — NimBLE tự gọi.
// ---------------------------------------------------------------------
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* server) override {
        connected = true;
        Serial.println("[BLE] Dien thoai DA KET NOI");
    }
    void onDisconnect(NimBLEServer* server) override {
        connected = false;
        Serial.println("[BLE] MAT KET NOI -> quang ba lai");
        NimBLEDevice::startAdvertising();   // Phát lại để điện thoại kết nối lại được
    }
};

// ---------------------------------------------------------------------
// Điện thoại ghi vào RX -> NimBLE gọi onWrite().
// ---------------------------------------------------------------------
class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        std::string v = c->getValue();                  // Dữ liệu điện thoại vừa gửi
        Serial.printf("[BLE] Nhan tu dien thoai: \"%s\"\n", v.c_str());
        if (v == "1") {
            set_led(true);                              // "1" -> bật LED
            Serial.println("      -> LED BAT");
        } else if (v == "0") {
            set_led(false);                             // "0" -> tắt LED
            Serial.println("      -> LED TAT");
        }
    }
};

// ---------------------------------------------------------------------
// Gửi một dòng chữ sang điện thoại, vd "dem=12 quai=DA CAI (doi)".
// ---------------------------------------------------------------------
static void send_status(bool buckled, const char* reason) {
    char msg[64];
    snprintf(msg, sizeof(msg), "dem=%lu quai=%s (%s)",
             (unsigned long)counter++, buckled ? "DA CAI" : "CHUA CAI", reason);

    tx_char->setValue((uint8_t*)msg, strlen(msg));   // Cập nhật giá trị (điện thoại bấm READ là thấy)
    if (connected) {
        tx_char->notify();                           // Có kết nối -> đẩy sang điện thoại
    }
    Serial.printf("[GUI] %s%s\n", msg, connected ? "" : "  (chua co ai ket noi)");
    last_send = millis();
}

void setup() {
    Serial.begin(115200);
    delay(1000);                          // Chờ cổng USB lên để không mất dòng in đầu
    Serial.println("=== ATGT BLE TEST ===");

    // Lưỡi gà: bật điện trở kéo lên nội, hở = HIGH, đóng (có nam châm) = LOW
#if REED_ACTIVE_LOW
    pinMode(REED_PIN, INPUT_PULLUP);
#else
    pinMode(REED_PIN, INPUT_PULLDOWN);
#endif
    reed_last_raw = reed_read_raw();      // Trạng thái lúc khởi động
    reed_stable = reed_last_raw;
    reed_changed_at = millis();

    pinMode(LED_PIN, OUTPUT);
    set_led(false);                       // Tắt LED lúc đầu

    // --- Dựng BLE ---
    NimBLEDevice::init(DEVICE_NAME);                       // Khởi động BLE, đặt tên
    NimBLEServer* server = NimBLEDevice::createServer();   // ESP32 làm server
    server->setCallbacks(new ServerCallbacks());

    NimBLEService* service = server->createService(SERVICE_UUID);
    tx_char = service->createCharacteristic(
        TX_CHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* rx_char = service->createCharacteristic(
        RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    rx_char->setCallbacks(new RxCallbacks());
    service->start();

    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(SERVICE_UUID);
    adv->setScanResponse(true);   // Tên nằm trong scan response
    adv->start();                 // Bắt đầu phát -> điện thoại quét là thấy "ATGT-TEST"

    Serial.printf("[BLE] Dang quang ba ten: %s\n", DEVICE_NAME);
    Serial.printf("[BLE] MAC: %s\n", NimBLEDevice::getAddress().toString().c_str());
}

void loop() {
    if (reed_update()) {
        send_status(reed_stable, "doi");            // Lưỡi gà vừa đổi (đã lọc dội) -> gửi ngay
    } else if (millis() - last_send >= NOTIFY_INTERVAL_MS) {
        send_status(reed_stable, "nhip");           // Không đổi -> mỗi 1 giây gửi lại (nhịp tim)
    }

    delay(10);
}
