/**
 * @file filter_median.h
 * @brief Bộ lọc trung vị cố định cho cửa sổ năm mẫu uint16_t.
 *
 * Mỗi nguồn dữ liệu phải dùng một Filter_Median_t riêng. Trong luồng sonar,
 * AppControl tạo ba phần tử tương ứng trước giữa, sau trái và sau phải.
 */

#ifndef FILTER_MEDIAN_H
#define FILTER_MEDIAN_H

#include <stdint.h>

/** Số mẫu cố định trong cửa sổ median. */
#define MEDIAN_FILTER_SIZE 5U

/**
 * @brief Trạng thái lưu năm mẫu gần nhất của một kênh.
 */
typedef struct
{
    uint16_t buffer[MEDIAN_FILTER_SIZE]; /* Vòng đệm năm mẫu gần nhất. */
    uint8_t index;                       /* Vị trí sẽ ghi ở lần Update tiếp. */
} Filter_Median_t;

/**
 * @brief Lấp toàn bộ cửa sổ bằng một giá trị ban đầu.
 *
 * Với cảm biến siêu âm, nên dùng mẫu Echo hợp lệ đầu tiên làm initial_val.
 *
 * @param filter Bộ lọc cần khởi tạo.
 * @param initial_val Giá trị an toàn dùng cho cả năm phần tử ban đầu.
 */
void Filter_Median_Init(Filter_Median_t *filter, uint16_t initial_val);

/**
 * @brief Thêm một mẫu và trả về trung vị của năm mẫu gần nhất.
 *
 * @param filter Bộ lọc cần cập nhật.
 * @param new_val Mẫu mới.
 * @return Trung vị hiện tại; trả 0 nếu filter bằng NULL.
 */
uint16_t Filter_Median_Update(Filter_Median_t *filter, uint16_t new_val);

#endif /* FILTER_MEDIAN_H */
