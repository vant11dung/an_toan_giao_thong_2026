// =====================================================================
// sleep_manager.cpp — cho ESP32 ngủ sâu và hẹn cách thức dậy.
//
// Ngủ sâu = tắt gần hết chip (CPU, Bluetooth, RAM thường), chỉ còn một
// mạch nhỏ canh giờ và canh chân GPIO. Thức dậy giống như vừa bấm reset:
// chạy lại từ setup(). Chỉ biến có RTC_DATA_ATTR là còn giữ giá trị.
// =====================================================================

#include "sleep_manager.h"

#include <esp_sleep.h>

#include "config.h"

void sleep_now(bool buckled, uint32_t timer_s) {
    // Thức khi chân quai đổi mức:
    //   đang cài (chân LOW)  -> thức khi chân lên HIGH (tháo quai)
    //   chưa cài (chân HIGH) -> thức khi chân về LOW  (cài quai)
    // Nếu quai đổi đúng lúc sắp ngủ thì chân đã ở mức đánh thức -> chip thức ngay,
    // đọc lại và gửi trạng thái mới. Không bị lỡ.
    esp_deep_sleep_enable_gpio_wakeup(
        1ULL << BUCKLE_PIN,
        buckled ? ESP_GPIO_WAKEUP_GPIO_HIGH : ESP_GPIO_WAKEUP_GPIO_LOW);

    if (timer_s > 0) {
        esp_sleep_enable_timer_wakeup((uint64_t)timer_s * 1000000ULL);   // đơn vị micro giây
    }

    esp_deep_sleep_start();   // Ngủ. Không bao giờ chạy tới dòng sau.
}
