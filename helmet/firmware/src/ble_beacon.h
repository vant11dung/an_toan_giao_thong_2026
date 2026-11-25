#pragma once
#include <stdint.h>

// Phát gói quảng bá trạng thái mũ trong ADV_BURST_MS (~0.3 giây) rồi dừng.
// Hàm chờ phát xong mới trả về.
void beacon_send(bool buckled, uint8_t battery, uint8_t seq);
