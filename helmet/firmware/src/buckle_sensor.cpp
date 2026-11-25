// =====================================================================
// buckle_sensor.cpp — đọc công tắc từ ở quai, có LỌC DỘI.
//
// Lọc dội: công tắc khi đóng/mở sẽ "nảy" vài lần trong vài ms (0-1-0-1-1...).
// Cách lọc: đọc liên tục, chỉ tin khi giá trị đứng yên đủ 50 ms.
// =====================================================================

#include "buckle_sensor.h"

#include <Arduino.h>
#include <driver/gpio.h>   // gpio_hold_dis

#include "config.h"

// Đọc chân 1 lần. Công tắc nối chân với GND: cài quai -> LOW -> true.
static bool read_raw() {
    return digitalRead(BUCKLE_PIN) == LOW;
}

void buckle_begin() {
    // Trước khi ngủ, thư viện ESP "khoá" cấu hình chân (hold) để giữ nguyên lúc ngủ.
    // Thức dậy phải mở khoá thì mới cấu hình lại được.
    gpio_hold_dis((gpio_num_t)BUCKLE_PIN);
    // Bật kéo lên nội (thêm vào điện trở ngoài 10 kΩ): công tắc hở -> chân HIGH
    pinMode(BUCKLE_PIN, INPUT_PULLUP);
}

bool buckle_read_stable() {
    bool value = read_raw();              // Giá trị đang xét
    uint32_t start = millis();            // Lúc bắt đầu đọc
    uint32_t stable_since = start;        // Lúc giá trị bắt đầu đứng yên

    // Lặp tới khi giá trị đứng yên đủ BUCKLE_STABLE_MS, hoặc quá thời gian tối đa
    while (millis() - stable_since < BUCKLE_STABLE_MS &&
           millis() - start < BUCKLE_READ_TIMEOUT_MS) {
        delay(5);
        bool now = read_raw();
        if (now != value) {               // Vừa nảy -> đổi giá trị, đếm lại từ đầu
            value = now;
            stable_since = millis();
        }
    }
    return value;
}
