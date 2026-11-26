// =====================================================================
// main.cpp — chương trình chính của ESP32-C3 trên mũ.
//
// Mỗi lần thức dậy (do cài/tháo quai, hoặc hẹn giờ) làm đúng 4 việc:
//   1. Đọc công tắc quai (có lọc dội)
//   2. Phát gói quảng bá ~0.3 giây
//   3. Tính lần thức tiếp theo
//   4. Ngủ sâu
// Toàn bộ chạy trong setup(). loop() không bao giờ được chạy tới.
//
// 3 trạng thái (xem docs/KE_HOACH_MU_BAO_HIEM.md mục 4.2):
//   CHƯA CÀI : ngủ hẳn, chỉ thức khi cài quai
//   VỪA CÀI  : 2 phút đầu, thức mỗi 10 giây
//   ĐANG CÀI : sau đó, thức mỗi 5 phút
// =====================================================================

#include <Arduino.h>
#include <esp_sleep.h>

#include "ble_beacon.h"
#include "buckle_sensor.h"
#include "config.h"
#include "helmet_protocol.h"
#include "sleep_manager.h"

#if DEBUG_SERIAL
#define LOG(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

// --- Biến trong bộ nhớ RTC: KHÔNG mất khi ngủ sâu (mất khi rút pin) ---
RTC_DATA_ATTR static uint8_t seq = 0;                // Số thứ tự gói
RTC_DATA_ATTR static bool was_buckled = false;       // Trạng thái quai lần thức trước
RTC_DATA_ATTR static uint8_t fast_wakes_left = 0;    // Còn bao nhiêu lần thức kiểu 10 giây

// Đo % pin. Chưa có mạch chia áp để đo nên trả 255 = "chưa đo".
static uint8_t read_battery_percent() {
    return HELMET_BATTERY_UNKNOWN;
}

// Chữ mô tả lý do thức dậy, chỉ để in Serial
static const char* wake_reason() {
    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_GPIO:  return "quai doi";
        case ESP_SLEEP_WAKEUP_TIMER: return "hen gio";
        default:                     return "bat nguon";
    }
}

void setup() {
    setCpuFrequencyMhz(80);   // Hạ xung CPU cho đỡ tốn pin. BLE vẫn chạy được ở 80 MHz.

#if DEBUG_SERIAL
    Serial.begin(115200);
    // Lần bật nguồn đầu tiên: chờ 2 giây để kịp mở Serial monitor
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_UNDEFINED) delay(2000);
#endif

#if TEST_MODE
    // Chế độ thử: không chạy phần ngủ bên dưới, nhảy thẳng sang loop()
    Serial.println("=== CHE DO THU: khong ngu, phat BLE lien tuc ===");
    buckle_begin();
    return;
#endif

    // 1. Đọc quai
    buckle_begin();
    bool buckled = buckle_read_stable();

    if (buckled && !was_buckled) {
        fast_wakes_left = FAST_WAKES;   // Vừa cài quai -> 2 phút gửi dày
    }
    was_buckled = buckled;

    // 2. Phát quảng bá
    LOG("[%s] quai: %s, seq=%u\n", wake_reason(), buckled ? "DA CAI" : "CHUA CAI", seq);
    beacon_send(buckled, read_battery_percent(), seq);
    seq++;   // 255 + 1 tự quay về 0

    // 3. Tính lần thức tiếp theo
    uint32_t timer_s = 0;                  // Chưa cài: không hẹn giờ
    if (buckled) {
        if (fast_wakes_left > 0) {
            fast_wakes_left--;
            timer_s = FAST_PERIOD_S;
        } else {
            timer_s = SLOW_PERIOD_S;
        }
    }

    // 4. Ngủ
    if (timer_s > 0) {
        LOG("Ngu %lu giay (hoac toi khi thao quai)\n", (unsigned long)timer_s);
    } else {
        LOG("Ngu han, cho cai quai\n");
    }
#if DEBUG_SERIAL
    Serial.flush();
#endif
    sleep_now(buckled, timer_s);
}

void loop() {
#if TEST_MODE
    // Chế độ thử: mỗi ~0.5 giây đọc quai, phát BLE, in ra Serial
    bool buckled = buckle_read_stable();
    Serial.printf("[thu] quai: %s, seq=%u\n", buckled ? "DA CAI" : "CHUA CAI", seq);
    beacon_send(buckled, read_battery_percent(), seq);
    seq++;
    delay(200);
#endif
    // Khi TEST_MODE = 0 thì không bao giờ chạy tới đây: setup() luôn kết thúc bằng ngủ sâu.
}
