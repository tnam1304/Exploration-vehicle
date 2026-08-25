/* Bo loc trung vi 5 mau dung rieng cho tung sonar. */

#include "filter_median.h"

#include <stddef.h>

/* Doi cho hai phan tu khi sap xep mang mau. */
static inline void Filter_Median_Swap(uint16_t *a, uint16_t *b)
{
    if (*a > *b)
    {
        uint16_t temp;

        temp = *a;
        *a = *b;
        *b = temp;
    }
}

/*
 * Khởi tạo bộ đếm lọc trung vị bằng giá trị an toàn ban đầu.
 */
/* Khoi tao bo loc va nap gia tri ban dau. */
void Filter_Median_Init(Filter_Median_t *filter, uint16_t initial_val)
{
    uint8_t i;

    if (filter != NULL)
    {
        for (i = 0U; i < MEDIAN_FILTER_SIZE; i++)
        {
            filter->buffer[i] = initial_val;
        }

        filter->index = 0U;
    }
}

/*
 * Cập nhật bộ lọc trung vị 5 mẫu và trả về giá trị trung vị.
 */
/* Them mau moi va tra ve trung vi cua cua so. */
uint16_t Filter_Median_Update(Filter_Median_t *filter, uint16_t new_val)
{
    uint16_t a;
    uint16_t b;
    uint16_t c;
    uint16_t d;
    uint16_t e;
    uint16_t median;

    a = 0U;
    b = 0U;
    c = 0U;
    d = 0U;
    e = 0U;
    median = 0U;

    if (filter != NULL)
    {
        filter->buffer[filter->index] = new_val;
        filter->index = (uint8_t)((filter->index + 1U) % MEDIAN_FILTER_SIZE);

        a = filter->buffer[0];
        b = filter->buffer[1];
        c = filter->buffer[2];
        d = filter->buffer[3];
        e = filter->buffer[4];

        /* Mang so sanh 5 gia tri; c la gia tri trung vi sau khi sap xep. */
        Filter_Median_Swap(&a, &b);
        Filter_Median_Swap(&d, &e);
        Filter_Median_Swap(&a, &c);
        Filter_Median_Swap(&b, &c);
        Filter_Median_Swap(&a, &d);
        Filter_Median_Swap(&c, &d);
        Filter_Median_Swap(&b, &e);
        Filter_Median_Swap(&b, &c);
        Filter_Median_Swap(&d, &e);
        Filter_Median_Swap(&c, &d);

        median = c;
    }

    return median;
}

