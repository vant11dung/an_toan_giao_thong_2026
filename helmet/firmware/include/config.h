#pragma once

// =====================================================================
// config.h — mọi con số có thể chỉnh của firmware mũ nằm ở đây.
// =====================================================================

// ---------- Phần cứng ----------
// Chân đọc công tắc quai. ESP32-C3 chỉ đánh thức từ ngủ sâu được bằng GPIO 0–5.
// Đấu dây:  GPIO 4 ── công tắc từ ── GND
//           GPIO 4 ── điện trở 10 kΩ ── 3V3   (BẮT BUỘC, xem giải thích bên dưới)
// Cài quai = công tắc đóng = chân về LOW.
//
// Vì sao bắt buộc điện trở 10 kΩ?
//   Lúc ngủ sâu mà quai đang cài, thư viện ESP tự bật điện trở kéo XUỐNG ~45 kΩ
//   bên trong chip. Tháo quai ra thì chân vẫn bị kéo về LOW -> chip không thức.
//   Điện trở ngoài 10 kΩ kéo LÊN mạnh hơn hẳn 45 kΩ -> chân lên HIGH -> chip thức.
//   Nếu test tháo quai mà mũ không báo ngay thì đổi sang 4.7 kΩ.
#define BUCKLE_PIN 4

// Đọc lại chân cho tới khi giá trị đứng yên đủ chừng này ms (lọc dội)
#define BUCKLE_STABLE_MS 50
// Đọc lâu nhất chừng này ms, quá thì lấy giá trị cuối (phòng công tắc rung mãi)
#define BUCKLE_READ_TIMEOUT_MS 500

// ---------- BLE quảng bá ----------
// Tên mũ, hiện trong app nRF Connect. Pi lọc theo MAC chứ không theo tên.
#define HELMET_NAME "ATGT-HELMET-01"

// Mỗi lần thức dậy phát trong bao lâu (ms)
#define ADV_BURST_MS 300
// Khoảng cách giữa 2 gói, đơn vị 0.625 ms. 48 x 0.625 = 30 ms -> ~10 gói trong 300 ms
#define ADV_INTERVAL_UNITS 48

// ---------- Nhịp thức dậy khi đang cài quai ----------
#define FAST_PERIOD_S 10    // 2 phút đầu sau khi cài: thức mỗi 10 giây
#define FAST_WAKES 12       // 12 lần x 10 giây = 2 phút
#define SLOW_PERIOD_S 300   // sau đó: thức mỗi 5 phút

// ---------- Gỡ lỗi ----------
// 1 = in Serial để xem trên laptop. Khi chạy bằng pin thật thì đặt 0 cho đỡ tốn.
#define DEBUG_SERIAL 1

// 1 = CHẾ ĐỘ THỬ: không ngủ, phát BLE liên tục, in Serial mỗi giây.
//     Dùng để kiểm tra bằng điện thoại (nRF Connect) và Serial monitor.
// 0 = chạy thật: ngủ sâu, tiết kiệm pin. NHỚ đổi về 0 trước khi lắp lên mũ.
#define TEST_MODE 1
