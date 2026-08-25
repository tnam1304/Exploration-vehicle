#include "dev_sonar.h"

#include "dev_sonar_config.h"
#include "stm32f4xx.h"

#include <stddef.h>


/* CẤU HÌNH NGOẠI VI DÙNG RIÊNG CHO DRIVER */

#define DEV_SONAR_TIMER                   TIM2

#define DEV_SONAR_NO_ACTIVE_SENSOR        (-1)


/* KIỂU DỮ LIỆU NỘI BỘ */


typedef enum
{
    DEV_SONAR_CAPTURE_IDLE = 0,
    DEV_SONAR_CAPTURE_WAIT_RISING,
    DEV_SONAR_CAPTURE_WAIT_FALLING
} Dev_SonarCaptureState_t;

typedef struct
{
    GPIO_TypeDef *trigger_port;
    uint8_t trigger_pin;
    GPIO_TypeDef *echo_port;
    uint8_t echo_pin;
} Dev_SonarConfig_t;

typedef struct
{
    /*
     * Các trường volatile được cả vòng lặp chính và ngắt EXTI truy cập.
     * Chúng phải được đọc lại từ RAM ở mỗi lần sử dụng.
     */
    volatile Dev_SonarCaptureState_t capture_state;
    volatile uint32_t rise_time_us;
    volatile uint32_t captured_pulse_us;
    volatile uint8_t capture_ready;

    /* Kết quả công khai chỉ được hoàn thiện trong Dev_Sonar_Process(). */
    Dev_SonarMeasurement_t latest;
    uint8_t new_data;
} Dev_SonarRuntime_t;


/* BẢNG ĐĂNG KÝ VÀ BIẾN TRẠNG THÁI                                           */


static Dev_SonarConfig_t
    s_sonar_config[DEV_SONAR_MAX_SENSOR_COUNT];

static Dev_SonarRuntime_t
    s_sonar_runtime[DEV_SONAR_MAX_SENSOR_COUNT];

static uint8_t s_sensor_count = 0U;

static volatile int8_t s_active_sensor = DEV_SONAR_NO_ACTIVE_SENSOR;

static uint8_t s_scan_mask = 0U;

static uint8_t s_next_sensor = 0U;

static uint32_t s_measurement_start_us = 0UL;

static uint32_t s_last_trigger_us = 0UL;


/* HÀM CẤU HÌNH CLOCK VÀ GPIO                                                 */


/* Bat clock cho GPIO port duoc dang ky. */
static void Dev_Sonar_EnableGpioClock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    }
    else if (port == GPIOB)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    }
    else if (port == GPIOC)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    }
    else if (port == GPIOD)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    }
    else if (port == GPIOE)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    }
    else if (port == GPIOH)
    {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    }
    else
    {
        /* Port không hợp lệ sẽ không được cấp clock. */
    }

    /*
     * Đọc lại thanh ghi để chắc chắn thao tác cấp clock đã hoàn tất trước
     * khi driver truy cập các thanh ghi GPIO.
     */
    (void)RCC->AHB1ENR;
}

/* Doi GPIO port sang ma port cua SYSCFG EXTI. */
static uint32_t Dev_Sonar_GetExtiPortCode(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        return 0UL;
    }
    if (port == GPIOB)
    {
        return 1UL;
    }
    if (port == GPIOC)
    {
        return 2UL;
    }
    if (port == GPIOD)
    {
        return 3UL;
    }
    if (port == GPIOE)
    {
        return 4UL;
    }
    if (port == GPIOH)
    {
        return 7UL;
    }

    /* Trả về GPIOA nếu nhận một port không nằm trong danh sách hỗ trợ. */
    return 0UL;
}

/* Kiem tra GPIO port co duoc STM32F401 ho tro. */
static bool Dev_Sonar_IsSupportedPort(GPIO_TypeDef *port)
{
    return ((port == GPIOA) ||
            (port == GPIOB) ||
            (port == GPIOC) ||
            (port == GPIOD) ||
            (port == GPIOE) ||
            (port == GPIOH));
}

/* Chan da duoc mot cam bien khac dang ky hay chua. */
static bool Dev_Sonar_IsPinAlreadyUsed(GPIO_TypeDef *port, uint8_t pin)
{
    uint8_t sensor_index;

    for (sensor_index = 0U;
         sensor_index < s_sensor_count;
         sensor_index++)
    {
        const Dev_SonarConfig_t *config = &s_sonar_config[sensor_index];

        if (((config->trigger_port == port) &&
             (config->trigger_pin == pin)) ||
            ((config->echo_port == port) &&
             (config->echo_pin == pin)))
        {
            return true;
        }
    }

    return false;
}

/* Line EXTI Echo da duoc mot cam bien khac su dung hay chua. */
static bool Dev_Sonar_IsEchoLineAlreadyUsed(uint8_t echo_pin)
{
    uint8_t sensor_index;

    for (sensor_index = 0U;
         sensor_index < s_sensor_count;
         sensor_index++)
    {
        if (s_sonar_config[sensor_index].echo_pin == echo_pin)
        {
            return true;
        }
    }

    return false;
}

/* Xoa trang thai do cua mot cam bien. */
static void Dev_Sonar_ResetRuntime(Dev_SonarRuntime_t *runtime)
{
    runtime->capture_state = DEV_SONAR_CAPTURE_IDLE;
    runtime->rise_time_us = 0UL;
    runtime->captured_pulse_us = 0UL;
    runtime->capture_ready = 0U;
    runtime->latest.distance_mm = 0U;
    runtime->latest.pulse_width_us = 0UL;
    runtime->latest.valid = false;
    runtime->new_data = 0U;
}

/* Tinh clock thuc cap cho TIM2. */
static uint32_t Dev_Sonar_GetTimerClockHz(void)
{
    const uint32_t ppre1_bits =
        (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    uint32_t apb1_divider;
    uint32_t timer_clock_hz;

    /* Giải mã ba bit PPRE1 thành hệ số chia clock APB1. */
    switch (ppre1_bits)
    {
        case 4UL:
            apb1_divider = 2UL;
            break;

        case 5UL:
            apb1_divider = 4UL;
            break;

        case 6UL:
            apb1_divider = 8UL;
            break;

        case 7UL:
            apb1_divider = 16UL;
            break;

        default:
            apb1_divider = 1UL;
            break;
    }

    timer_clock_hz = SystemCoreClock / apb1_divider;

    /*
     * Trên STM32F4, timer thuộc APB được nhân đôi clock khi APB prescaler
     * khác 1. Project hiện tại chạy TIM2 với clock 84 MHz.
     */
    if (apb1_divider != 1UL)
    {
        timer_clock_hz *= 2UL;
    }

    return timer_clock_hz;
}

/* Cau hinh TIM2 thanh bo dem 1 us. */
static void Dev_Sonar_TimerInit(void)
{
    const uint32_t timer_clock_hz = Dev_Sonar_GetTimerClockHz();
    uint32_t prescaler;

    /* Bật clock TIM2 và đọc lại thanh ghi để chờ clock ổn định. */
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    /* Tính PSC sao cho tần số đếm bằng DEV_SONAR_TIMER_TICK_HZ. */
    prescaler = timer_clock_hz / DEV_SONAR_TIMER_TICK_HZ;
    if (prescaler == 0UL)
    {
        prescaler = 1UL;
    }

    /* Dừng và cấu hình lại toàn bộ TIM2 trước khi bắt đầu đếm. */
    DEV_SONAR_TIMER->CR1 = 0UL;
    DEV_SONAR_TIMER->PSC = prescaler - 1UL;
    DEV_SONAR_TIMER->ARR = 0xFFFFFFFFUL;
    DEV_SONAR_TIMER->CNT = 0UL;

    /* Nạp PSC/ARR ngay lập tức, xóa cờ rồi cho timer chạy. */
    DEV_SONAR_TIMER->EGR = TIM_EGR_UG;
    DEV_SONAR_TIMER->SR = 0UL;
    DEV_SONAR_TIMER->CR1 = TIM_CR1_CEN;
}

/* Bat dung IRQ theo so EXTI cua chan Echo. */
static void Dev_Sonar_EnableExtiIrq(uint8_t echo_pin)
{
    IRQn_Type irq;

    if (echo_pin == 0U)
    {
        irq = EXTI0_IRQn;
    }
    else if (echo_pin == 1U)
    {
        irq = EXTI1_IRQn;
    }
    else if (echo_pin == 2U)
    {
        irq = EXTI2_IRQn;
    }
    else if (echo_pin == 3U)
    {
        irq = EXTI3_IRQn;
    }
    else if (echo_pin == 4U)
    {
        irq = EXTI4_IRQn;
    }
    else if (echo_pin <= 9U)
    {
        irq = EXTI9_5_IRQn;
    }
    else
    {
        irq = EXTI15_10_IRQn;
    }

    NVIC_SetPriority(irq, DEV_SONAR_EXTI_PRIORITY);
    NVIC_EnableIRQ(irq);
}

/* Cau hinh Trigger, Echo va EXTI cho mot cam bien. */
static void Dev_Sonar_PinInit(const Dev_SonarConfig_t *config)
{
    const uint32_t trigger_shift = (uint32_t)config->trigger_pin * 2UL;
    const uint32_t echo_shift = (uint32_t)config->echo_pin * 2UL;
    const uint32_t echo_mask = 1UL << config->echo_pin;
    const uint32_t exti_index = (uint32_t)config->echo_pin / 4UL;
    const uint32_t exti_shift =
        ((uint32_t)config->echo_pin % 4UL) * 4UL;
    const uint32_t exti_port_code =
        Dev_Sonar_GetExtiPortCode(config->echo_port);

    /* Bật clock cho cả hai port trước khi cấu hình thanh ghi GPIO. */
    Dev_Sonar_EnableGpioClock(config->trigger_port);
    Dev_Sonar_EnableGpioClock(config->echo_port);

    /*
     * Trigger:
     * - Output push-pull.
     * - Tốc độ trung bình.
     * - Không pull-up/pull-down.
     * - Mặc định giữ mức LOW.
     */
    config->trigger_port->MODER =
        (config->trigger_port->MODER & ~(3UL << trigger_shift)) |
        (1UL << trigger_shift);
    config->trigger_port->OTYPER &=
        ~(1UL << config->trigger_pin);
    config->trigger_port->OSPEEDR =
        (config->trigger_port->OSPEEDR & ~(3UL << trigger_shift)) |
        (1UL << trigger_shift);
    config->trigger_port->PUPDR &=
        ~(3UL << trigger_shift);
    config->trigger_port->BSRR =
        1UL << ((uint32_t)config->trigger_pin + 16UL);

    /*
     * Echo:
     * - Input.
     * - Pull-down để chân không bị trôi khi tháo cảm biến.
     */
    config->echo_port->MODER &=
        ~(3UL << echo_shift);
    config->echo_port->PUPDR =
        (config->echo_port->PUPDR & ~(3UL << echo_shift)) |
        (2UL << echo_shift);

    /* Bật clock SYSCFG để ánh xạ GPIO port vào đúng line EXTI. */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    /* Chỉ thay bốn bit của line EXTI đang cấu hình. */
    SYSCFG->EXTICR[exti_index] =
        (SYSCFG->EXTICR[exti_index] & ~(0xFUL << exti_shift)) |
        (exti_port_code << exti_shift);

    /*
     * Cho phép ngắt ở cả cạnh lên và cạnh xuống. State machine sẽ kiểm tra
     * mức thực tế trên IDR để xác định cạnh nào vừa xảy ra.
     */
    EXTI->IMR |= echo_mask;
    EXTI->RTSR |= echo_mask;
    EXTI->FTSR |= echo_mask;

    /* Xóa cờ pending cũ trước khi bật NVIC. */
    EXTI->PR = echo_mask;

    /* Bật đúng vector ngắt theo chân Echo đã đăng ký. */
    Dev_Sonar_EnableExtiIrq(config->echo_pin);
}


/* HÀM PHÁT TRIGGER VÀ LỊCH QUÉT                                              */


/* Phat xung Trigger 10 us cho mot cam bien. */
static void Dev_Sonar_SendTrigger(uint8_t sensor_index)
{
    const Dev_SonarConfig_t *config = &s_sonar_config[sensor_index];
    Dev_SonarRuntime_t *runtime = &s_sonar_runtime[sensor_index];
    const uint32_t echo_mask = 1UL << config->echo_pin;
    uint32_t trigger_start_us;

    /* Chuẩn bị state machine bắt đầu chờ cạnh lên của Echo. */
    runtime->capture_state = DEV_SONAR_CAPTURE_WAIT_RISING;
    runtime->rise_time_us = 0UL;
    runtime->captured_pulse_us = 0UL;
    runtime->capture_ready = 0U;

    /* Xóa cờ Echo cũ và đánh dấu cảm biến đang được phép trả Echo. */
    EXTI->PR = echo_mask;
    s_active_sensor = (int8_t)sensor_index;

    /* Đưa Trigger lên HIGH và ghi lại thời điểm bắt đầu xung. */
    config->trigger_port->BSRR = 1UL << config->trigger_pin;
    trigger_start_us = DEV_SONAR_TIMER->CNT;

    /* Chỉ block trong thời gian rất ngắn của xung Trigger. */
    while ((uint32_t)(DEV_SONAR_TIMER->CNT - trigger_start_us) <
           DEV_SONAR_TRIGGER_PULSE_US)
    {
        /* Không thực hiện công việc khác trong 10 us này. */
    }

    /* Kết thúc Trigger bằng cách kéo chân về LOW. */
    config->trigger_port->BSRR =
        1UL << ((uint32_t)config->trigger_pin + 16UL);

    /* Bắt đầu tính timeout sau khi xung Trigger đã kết thúc. */
    s_measurement_start_us = DEV_SONAR_TIMER->CNT;
    s_last_trigger_us = s_measurement_start_us;
}

/* Tim cam bien tiep theo dang duoc bat trong scan mask. */
static int8_t Dev_Sonar_FindNextSensor(void)
{
    uint8_t offset;

    /* Không thực hiện phép chia lấy dư khi chưa đăng ký cảm biến. */
    if (s_sensor_count == 0U)
    {
        return DEV_SONAR_NO_ACTIVE_SENSOR;
    }

    /* Duyệt tối đa sensor_count lần để giữ thứ tự round-robin. */
    for (offset = 0U; offset < s_sensor_count; offset++)
    {
        const uint8_t sensor_index =
            (uint8_t)((s_next_sensor + offset) % s_sensor_count);

        if ((s_scan_mask & DEV_SONAR_MASK(sensor_index)) != 0U)
        {
            /* Lần sau bắt đầu tìm từ cảm biến đứng sau cảm biến vừa chọn. */
            s_next_sensor =
                (uint8_t)((sensor_index + 1U) % s_sensor_count);
            return (int8_t)sensor_index;
        }
    }

    return DEV_SONAR_NO_ACTIVE_SENSOR;
}


/* HÀM XỬ LÝ KẾT QUẢ                                                         */


/* Doi do rong Echo sang khoang cach mm. */
static uint32_t Dev_Sonar_PulseToMillimeters(uint32_t pulse_width_us)
{
    return ((pulse_width_us * 343UL) + 1000UL) / 2000UL;
}

/* Chot ket qua khi da nhan du hai canh Echo. */
static void Dev_Sonar_FinishCapture(uint8_t sensor_index)
{
    Dev_SonarRuntime_t *runtime = &s_sonar_runtime[sensor_index];
    const uint32_t pulse_width_us = runtime->captured_pulse_us;
    const uint32_t distance_mm =
        Dev_Sonar_PulseToMillimeters(pulse_width_us);

    /* Reset phần bắt cạnh để cảm biến trở về trạng thái rảnh. */
    runtime->capture_ready = 0U;
    runtime->capture_state = DEV_SONAR_CAPTURE_IDLE;

    /* Luôn lưu độ rộng xung để hỗ trợ kiểm tra lỗi phần cứng. */
    runtime->latest.pulse_width_us = pulse_width_us;

    /* Chỉ công nhận kết quả nằm trong cả giới hạn xung và khoảng cách. */
    if ((pulse_width_us >= DEV_SONAR_MIN_ECHO_PULSE_US) &&
        (pulse_width_us <= DEV_SONAR_ECHO_TIMEOUT_US) &&
        (distance_mm <= DEV_SONAR_MAX_DISTANCE_MM))
    {
        runtime->latest.distance_mm = (uint16_t)distance_mm;
        runtime->latest.valid = true;
    }
    else
    {
        runtime->latest.distance_mm = 0U;
        runtime->latest.valid = false;
    }

    /* Báo cho tầng gọi biết cảm biến này vừa có một kết quả mới. */
    runtime->new_data = 1U;
    s_active_sensor = DEV_SONAR_NO_ACTIVE_SENSOR;
}

/* Chot ket qua loi khi qua thoi gian cho Echo. */
static void Dev_Sonar_FinishTimeout(uint8_t sensor_index)
{
    Dev_SonarRuntime_t *runtime = &s_sonar_runtime[sensor_index];

    /* Timeout không giữ khoảng cách cũ để tránh dùng nhầm dữ liệu đã lỗi thời. */
    runtime->capture_ready = 0U;
    runtime->capture_state = DEV_SONAR_CAPTURE_IDLE;
    runtime->latest.distance_mm = 0U;
    runtime->latest.pulse_width_us = 0UL;
    runtime->latest.valid = false;
    runtime->new_data = 1U;

    s_active_sensor = DEV_SONAR_NO_ACTIVE_SENSOR;
}


/* API CÔNG KHAI   */


/* Khoi tao timer va xoa toan bo bang cam bien. */
void Dev_Sonar_Init(void)
{
    uint8_t sensor_index;
    uint32_t now_after_init;

    /*
     * Nếu tầng ứng dụng chủ động khởi tạo lại driver, tắt các line Echo cũ
     * trước khi xóa bảng đăng ký. Thao tác này không ảnh hưởng EXTI0..EXTI4
     * hoặc EXTI10..EXTI15 của module khác.
     */
    for (sensor_index = 0U;
         sensor_index < s_sensor_count;
         sensor_index++)
    {
        const uint32_t echo_mask =
            1UL << s_sonar_config[sensor_index].echo_pin;

        EXTI->IMR &= ~echo_mask;
        EXTI->RTSR &= ~echo_mask;
        EXTI->FTSR &= ~echo_mask;
        EXTI->PR = echo_mask;
    }

    /* TIM2 phải chạy trước khi driver tạo xung Trigger hoặc đo Echo. */
    Dev_Sonar_TimerInit();

    /* Xóa toàn bộ bảng để chuẩn bị nhận các lệnh Dev_Sonar_Add(). */
    for (sensor_index = 0U;
         sensor_index < DEV_SONAR_MAX_SENSOR_COUNT;
         sensor_index++)
    {
        s_sonar_config[sensor_index].trigger_port = NULL;
        s_sonar_config[sensor_index].trigger_pin = 0U;
        s_sonar_config[sensor_index].echo_port = NULL;
        s_sonar_config[sensor_index].echo_pin = 0U;
        Dev_Sonar_ResetRuntime(&s_sonar_runtime[sensor_index]);
    }

    /* Thiết lập lịch quét ban đầu. */
    s_sensor_count = 0U;
    s_active_sensor = DEV_SONAR_NO_ACTIVE_SENSOR;
    s_scan_mask = 0U;
    s_next_sensor = 0U;
    s_measurement_start_us = 0UL;

    /*
     * Trừ trước một chu kỳ để lần gọi Process đầu tiên được phép phát ngay.
     * Phép trừ unsigned vẫn an toàn khi TIM2 chưa đếm đủ một chu kỳ.
     */
    now_after_init = DEV_SONAR_TIMER->CNT;
    s_last_trigger_us =
        now_after_init - DEV_SONAR_TRIGGER_INTERVAL_US;
}

/* Dang ky GPIO cua mot sonar va tra ve ID. */
Dev_SonarId_t Dev_Sonar_Add(
    GPIO_TypeDef *trigger_port,
    uint8_t trigger_pin,
    GPIO_TypeDef *echo_port,
    uint8_t echo_pin)
{
    Dev_SonarConfig_t *config;
    Dev_SonarId_t new_sensor_id;

    /*
     * Chỉ nhận port STM32 hợp lệ và số chân nằm trong giới hạn GPIO.
     * Điều này ngăn phép dịch bit sai hoặc truy cập địa chỉ port không hợp lệ.
     */
    if ((!Dev_Sonar_IsSupportedPort(trigger_port)) ||
        (!Dev_Sonar_IsSupportedPort(echo_port)) ||
        (trigger_pin > 15U) ||
        (echo_pin > 15U))
    {
        return DEV_SONAR_ID_INVALID;
    }

    /* Echo phải thuộc đúng nhóm EXTI mà handler cuối file đang quản lý. */
    if ((echo_pin < DEV_SONAR_ECHO_EXTI_LINE_MIN) ||
        (echo_pin > DEV_SONAR_ECHO_EXTI_LINE_MAX))
    {
        return DEV_SONAR_ID_INVALID;
    }

    /*
     * Không nhận thêm cấu hình khi bảng đã đầy hoặc trong lúc đang đo.
     * Cách dùng khuyến nghị là đăng ký toàn bộ cảm biến trước vòng lặp chính.
     */
    if ((s_sensor_count >= DEV_SONAR_MAX_SENSOR_COUNT) ||
        (s_active_sensor != DEV_SONAR_NO_ACTIVE_SENSOR))
    {
        return DEV_SONAR_ID_INVALID;
    }

    /* Trigger và Echo của cùng một cảm biến không được là cùng một chân. */
    if ((trigger_port == echo_port) && (trigger_pin == echo_pin))
    {
        return DEV_SONAR_ID_INVALID;
    }

    /* Không cho hai chức năng sonar dùng chung một chân GPIO. */
    if (Dev_Sonar_IsPinAlreadyUsed(trigger_port, trigger_pin) ||
        Dev_Sonar_IsPinAlreadyUsed(echo_port, echo_pin))
    {
        return DEV_SONAR_ID_INVALID;
    }

    /*
     * Hai chân khác port nhưng cùng số pin vẫn tranh chấp một line EXTI.
     * Ví dụ PC6 và PA6 không thể cùng được đăng ký làm Echo.
     */
    if (Dev_Sonar_IsEchoLineAlreadyUsed(echo_pin))
    {
        return DEV_SONAR_ID_INVALID;
    }

    /* ID được cấp tuần tự theo đúng thứ tự gọi Dev_Sonar_Add(). */
    new_sensor_id = (Dev_SonarId_t)s_sensor_count;
    config = &s_sonar_config[s_sensor_count];

    config->trigger_port = trigger_port;
    config->trigger_pin = trigger_pin;
    config->echo_port = echo_port;
    config->echo_pin = echo_pin;
    Dev_Sonar_ResetRuntime(&s_sonar_runtime[s_sensor_count]);

    /*
     * Tăng count trước khi bật NVIC để nếu có một cạnh nhiễu xuất hiện ngay,
     * handler vẫn nhìn thấy phần tử vừa đăng ký và xóa đúng cờ pending.
     */
    s_sensor_count++;
    Dev_Sonar_PinInit(config);

    /* Cảm biến mới mặc định được đưa vào lịch quét. */
    s_scan_mask |= DEV_SONAR_MASK(new_sensor_id);

    return new_sensor_id;
}

/* Chay scheduler, xu ly capture va timeout. */
void Dev_Sonar_Process(void)
{
    uint32_t now_us = DEV_SONAR_TIMER->CNT;
    bool measurement_completed = false;

    /* Trước tiên xử lý cảm biến đang chờ Echo, nếu có. */
    if (s_active_sensor != DEV_SONAR_NO_ACTIVE_SENSOR)
    {
        const uint8_t active_index = (uint8_t)s_active_sensor;
        Dev_SonarRuntime_t *runtime = &s_sonar_runtime[active_index];

        if (runtime->capture_ready != 0U)
        {
            /* Ngắt đã bắt đủ cạnh lên và cạnh xuống. */
            Dev_Sonar_FinishCapture(active_index);
            measurement_completed = true;
        }
        else if ((uint32_t)(now_us - s_measurement_start_us) >=
                 DEV_SONAR_ECHO_TIMEOUT_US)
        {
            /* Quá thời gian chờ nhưng chưa có một xung Echo hoàn chỉnh. */
            Dev_Sonar_FinishTimeout(active_index);
            measurement_completed = true;
        }
        else
        {
            /* Phép đo vẫn đang chờ Echo, chưa cần xử lý gì thêm. */
        }
    }

    /* Đọc lại thời gian vì phần xử lý phía trên có thể đã mất vài us. */
    now_us = DEV_SONAR_TIMER->CNT;

    /*
     * Chỉ phát cảm biến mới khi:
     * - Không còn cảm biến nào đang chờ Echo.
     * - Scan mask có ít nhất một cảm biến được bật.
     * - Đã đủ khoảng cách thời gian từ lần Trigger trước.
     */
    /* Chừa một vòng để AppControl đổi slot sau khi nhận kết quả mới. */
    if ((!measurement_completed) &&
        (s_active_sensor == DEV_SONAR_NO_ACTIVE_SENSOR) &&
        (s_scan_mask != 0U) &&
        ((uint32_t)(now_us - s_last_trigger_us) >=
         DEV_SONAR_TRIGGER_INTERVAL_US))
    {
        const int8_t next_sensor = Dev_Sonar_FindNextSensor();

        if (next_sensor != DEV_SONAR_NO_ACTIVE_SENSOR)
        {
            Dev_Sonar_SendTrigger((uint8_t)next_sensor);
        }
    }
}

/* Chon tap cam bien duoc phep quet. */
void Dev_Sonar_SetScanMask(uint8_t scan_mask)
{
    /* Loại bỏ mọi bit không thuộc các ID đã được đăng ký. */
    s_scan_mask =
        (uint8_t)(scan_mask & Dev_Sonar_GetRegisteredMask());
}

/* Doc scan mask hien tai. */
uint8_t Dev_Sonar_GetScanMask(void)
{
    return s_scan_mask;
}

/* Lay mask cua cac ID da dang ky. */
uint8_t Dev_Sonar_GetRegisteredMask(void)
{
    if (s_sensor_count == 0U)
    {
        return 0U;
    }

    return (uint8_t)((1UL << s_sensor_count) - 1UL);
}

/* Lay tong so sonar da dang ky. */
uint8_t Dev_Sonar_GetSensorCount(void)
{
    return s_sensor_count;
}

/* Doc ket qua gan nhat cua mot ID. */
bool Dev_Sonar_GetLatest(
    Dev_SonarId_t sensor_id,
    Dev_SonarMeasurement_t *measurement)
{
    Dev_SonarRuntime_t *runtime;

    /* Không truy cập mảng nếu ID hoặc con trỏ đầu ra không hợp lệ. */
    if ((sensor_id >= s_sensor_count) ||
        (measurement == NULL))
    {
        return false;
    }

    runtime = &s_sonar_runtime[sensor_id];
    *measurement = runtime->latest;

    return true;
}

/* Doc ket qua moi va xoa co new_data. */
bool Dev_Sonar_GetNewData(
    Dev_SonarId_t sensor_id,
    Dev_SonarMeasurement_t *measurement)
{
    Dev_SonarRuntime_t *runtime;

    /* Không truy cập mảng nếu ID hoặc con trỏ đầu ra không hợp lệ. */
    if ((sensor_id >= s_sensor_count) ||
        (measurement == NULL))
    {
        return false;
    }

    runtime = &s_sonar_runtime[sensor_id];

    /* Chưa có kết quả mới thì không thay đổi vùng nhớ đầu ra. */
    if (runtime->new_data == 0U)
    {
        return false;
    }

    /* Sao chép kết quả rồi chỉ xóa cờ của cảm biến vừa được đọc. */
    *measurement = runtime->latest;
    runtime->new_data = 0U;

    return true;
}


/* XỬ LÝ NGẮT ECHO */


/* Xu ly cac canh Echo tren cac line EXTI da dang ky. */
static void Dev_Sonar_HandleExtiInterrupt(void)
{
    /* Chụp một lần toàn bộ cờ pending để tránh đọc thay đổi giữa vòng lặp. */
    const uint32_t pending_lines = EXTI->PR;
    uint8_t sensor_index;

    /* Chỉ kiểm tra những phần tử đã được Dev_Sonar_Add() đăng ký. */
    for (sensor_index = 0U;
         sensor_index < s_sensor_count;
         sensor_index++)
    {
        const Dev_SonarConfig_t *config = &s_sonar_config[sensor_index];
        Dev_SonarRuntime_t *runtime = &s_sonar_runtime[sensor_index];
        const uint32_t echo_mask = 1UL << config->echo_pin;

        /* Bỏ qua cảm biến nếu line EXTI của nó không có cờ pending. */
        if ((pending_lines & echo_mask) == 0UL)
        {
            continue;
        }

        /* STM32F4 xóa cờ pending bằng cách ghi 1 vào bit tương ứng. */
        EXTI->PR = echo_mask;

        /*
         * Chỉ cảm biến vừa được Trigger mới được phép cập nhật phép đo.
         * Cạnh từ cảm biến khác được xóa nhưng không sử dụng.
         */
        if (s_active_sensor != (int8_t)sensor_index)
        {
            continue;
        }

        /*
         * Cạnh lên hợp lệ:
         * - State machine đang chờ cạnh lên.
         * - Mức thực tế trên chân Echo đang là HIGH.
         */
        if ((runtime->capture_state == DEV_SONAR_CAPTURE_WAIT_RISING) &&
            ((config->echo_port->IDR & echo_mask) != 0UL))
        {
            runtime->rise_time_us = DEV_SONAR_TIMER->CNT;
            runtime->capture_state = DEV_SONAR_CAPTURE_WAIT_FALLING;
        }
        /*
         * Cạnh xuống hợp lệ:
         * - Trước đó đã bắt được cạnh lên.
         * - Mức thực tế trên chân Echo đã về LOW.
         */
        else if ((runtime->capture_state == DEV_SONAR_CAPTURE_WAIT_FALLING) &&
                 ((config->echo_port->IDR & echo_mask) == 0UL))
        {
            /*
             * Phép trừ uint32_t tự xử lý đúng cả trường hợp TIM2 tràn CNT
             * giữa cạnh lên và cạnh xuống.
             */
            runtime->captured_pulse_us =
                (uint32_t)(DEV_SONAR_TIMER->CNT - runtime->rise_time_us);
            runtime->capture_ready = 1U;
            runtime->capture_state = DEV_SONAR_CAPTURE_IDLE;
        }
        else
        {
            /* Cạnh không phù hợp với state hiện tại được bỏ qua. */
        }
    }
}

/* Handler rieng cua driver, khong sua file ngat Cube. */
void EXTI9_5_IRQHandler(void)
{
    Dev_Sonar_HandleExtiInterrupt();
}

/* Xu ly Echo Front tren PB2 / EXTI2. */
void EXTI2_IRQHandler(void)
{
    Dev_Sonar_HandleExtiInterrupt();
}

/* Xu ly Echo Rear Left tren PB3 / EXTI3. */
void EXTI3_IRQHandler(void)
{
    Dev_Sonar_HandleExtiInterrupt();
}

/* Xu ly Echo Rear Right tren PB4 / EXTI4. */
void EXTI4_IRQHandler(void)
{
    Dev_Sonar_HandleExtiInterrupt();
}

