#pragma once
#include <stdint.h>

// Ngủ sâu. Thức dậy khi quai ĐỔI trạng thái so với `buckled`,
// hoặc sau `timer_s` giây (0 = không hẹn giờ, chỉ thức khi quai đổi).
// Hàm này không bao giờ trả về: thức dậy là chạy lại từ setup().
void sleep_now(bool buckled, uint32_t timer_s);
