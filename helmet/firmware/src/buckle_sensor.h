#pragma once

// Cấu hình chân công tắc quai. Gọi 1 lần mỗi lần thức dậy.
void buckle_begin();

// Đọc công tắc có lọc dội (chờ giá trị đứng yên ~50 ms). true = đã cài quai.
bool buckle_read_stable();
