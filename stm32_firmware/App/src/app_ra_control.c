/* Dieu phoi ba sonar, median filter va dau vao Reverse Assist. */

#include "app_ra_control.h"
#include "bsp_pinout.h"
#include "app_ra_config.h"
#include "dev_sonar.h"
#include "filter_median.h"

#include <stddef.h>

/*
 * Reverse Assist dùng số mẫu này để tính độ trễ phản ứng. Dừng biên dịch nếu cấu
 * hình an toàn và kích thước thật của median filter bị sửa lệch nhau.
 */
#if (MEDIAN_FILTER_SIZE != RA_MEDIAN_SIZE)
#error "Median filter size does not match Reverse Assist configuration"
#endif


/* KIỂU DỮ LIỆU NỘI BỘ                                                       */


typedef struct
{
    Dev_SonarId_t driver_id;          /* ID do Dev_Sonar_Add() tra ve. */
    Filter_Median_t median_filter;    /* Bo loc rieng cua cam bien. */
    bool filter_initialized;          /* Da nhan mau Echo hop le dau tien. */
    SonarData_t public_data;          /* Du lieu cm cho tang tren. */
} SonarChannel_t;


/* BIẾN TRẠNG THÁI                                                           */


static SonarChannel_t s_sonar_channels[SONAR_COUNT];

static RA_Input_t s_safety_input;

static RA_Output_t s_safety_output;

static bool s_control_ready = false;

static const SonarPosition_t s_sonar_schedule[] =
{
    SONAR_FRONT,
    SONAR_FRONT,
    SONAR_REAR_L,
    SONAR_FRONT,
    SONAR_FRONT,
    SONAR_REAR_R
};

#define SONAR_SCHEDULE_COUNT \
    ((uint8_t)(sizeof(s_sonar_schedule) / \
               sizeof(s_sonar_schedule[0])))

static uint8_t s_sonar_schedule_index = 0U;


/* HÀM HỖ TRỢ KHỞI TẠO                                                       */


/* Xoa du lieu va bo loc cua mot kenh sonar. */
static void RA_Control_ResetSonarChannel(
    SonarChannel_t *channel)
{
    if (channel == NULL)
    {
        return;
    }

    channel->driver_id = DEV_SONAR_ID_INVALID;
    channel->filter_initialized = false;
    channel->public_data.raw_distance_cm = 0.0f;
    channel->public_data.filtered_distance_cm = 0.0f;
    channel->public_data.valid = false;
    channel->public_data.has_sample = false;
    channel->public_data.sample_sequence = 0UL;
}

/* Tao scan mask chi chua cam bien truoc. */
static uint8_t RA_Control_GetFrontScanMask(void)
{
    const Dev_SonarId_t front_id =
        s_sonar_channels[SONAR_FRONT].driver_id;

    if (front_id == DEV_SONAR_ID_INVALID)
    {
        return 0U;
    }

    return DEV_SONAR_MASK(front_id);
}

/* Doi vi tri vat ly sang scan mask cua driver. */
static uint8_t RA_Control_GetPositionScanMask(
    SonarPosition_t position)
{
    const Dev_SonarId_t driver_id =
        s_sonar_channels[(uint32_t)position].driver_id;

    if (driver_id == DEV_SONAR_ID_INVALID)
    {
        return 0U;
    }

    return DEV_SONAR_MASK(driver_id);
}

/* Khoi dong lai lich FRONT-FRONT-REAR theo slot dau. */
static void RA_Control_ResetSonarScheduler(void)
{
    s_sonar_schedule_index = 0U;
    Dev_Sonar_SetScanMask(RA_Control_GetFrontScanMask());
}

/* Tao scan mask cho hai cam bien sau. */
static uint8_t RA_Control_GetRearScanMask(void)
{
    const Dev_SonarId_t rear_left_id =
        s_sonar_channels[SONAR_REAR_L].driver_id;
    const Dev_SonarId_t rear_right_id =
        s_sonar_channels[SONAR_REAR_R].driver_id;
    uint8_t scan_mask = 0U;

    if (rear_left_id != DEV_SONAR_ID_INVALID)
    {
        scan_mask |= DEV_SONAR_MASK(rear_left_id);
    }

    if (rear_right_id != DEV_SONAR_ID_INVALID)
    {
        scan_mask |= DEV_SONAR_MASK(rear_right_id);
    }

    return scan_mask;
}

/* Danh dau kenh sonar cu la chua co du lieu moi. */
static void RA_Control_InvalidateSonarChannel(
    SonarPosition_t position)
{
    SonarChannel_t *channel = &s_sonar_channels[(uint32_t)position];

    channel->filter_initialized = false;
    channel->public_data.valid = false;
    channel->public_data.has_sample = false;
}


/* HÀM HỖ TRỢ XỬ LÝ MẪU SONAR                                                */


/* Lay dung mot ket qua moi tu driver va cap nhat median. */
static bool RA_Control_UpdateSonarChannel(
    SonarChannel_t *channel)
{
    Dev_SonarMeasurement_t measurement;
    uint16_t filtered_distance_mm;

    if ((channel == NULL) ||
        (channel->driver_id == DEV_SONAR_ID_INVALID))
    {
        return false;
    }

    /* Chỉ lọc khi driver báo đúng một kết quả mới. */
    if (!Dev_Sonar_GetNewData(channel->driver_id, &measurement))
    {
        return false;
    }

    channel->public_data.has_sample = true;
    channel->public_data.sample_sequence++;

    /*
     * Không đưa TIMEOUT dưới dạng 0 vào bộ lọc vì 0 có thể bị hiểu nhầm là
     * vật cản sát đầu xe và tạo yêu cầu AEB sai.
     */
    if (!measurement.valid)
    {
        channel->filter_initialized = false;
        channel->public_data.raw_distance_cm = 0.0f;
        channel->public_data.filtered_distance_cm = 0.0f;
        channel->public_data.valid = false;
        return true;
    }

    /*
     * Median filter nhận trực tiếp milimét vì dữ liệu là uint16_t.
     * Mẫu hợp lệ đầu tiên được dùng để lấp cả năm phần tử, tránh hai hoặc ba
     * kết quả 0 giả ở lúc mới khởi động.
     */
    if (!channel->filter_initialized)
    {
        Filter_Median_Init(
            &channel->median_filter,
            measurement.distance_mm);

        filtered_distance_mm = measurement.distance_mm;
        channel->filter_initialized = true;
    }
    else
    {
        filtered_distance_mm =
            Filter_Median_Update(
                &channel->median_filter,
                measurement.distance_mm);
    }

    /* Đổi mm sang cm đúng một lần tại ranh giới tầng ứng dụng. */
    channel->public_data.raw_distance_cm =
        (float)measurement.distance_mm / 10.0f;
    channel->public_data.filtered_distance_cm =
        (float)filtered_distance_mm / 10.0f;
    channel->public_data.valid = true;
    return true;
}

/* Chep ba khoang cach da loc vao dau vao Reverse Assist. */
static void RA_Control_UpdateSafetyInputFromSonar(void)
{
    const SonarData_t *front_data =
        &s_sonar_channels[SONAR_FRONT].public_data;
    const SonarData_t *rear_left_data =
        &s_sonar_channels[SONAR_REAR_L].public_data;
    const SonarData_t *rear_right_data =
        &s_sonar_channels[SONAR_REAR_R].public_data;

    s_safety_input.front_distance_cm =
        front_data->filtered_distance_cm;
    s_safety_input.front_distance_valid =
        front_data->valid;
    s_safety_input.front_sample_sequence =
        front_data->sample_sequence;

    s_safety_input.rear_left_distance_cm =
        rear_left_data->filtered_distance_cm;
    s_safety_input.rear_left_distance_valid =
        rear_left_data->valid;
    s_safety_input.rear_left_sample_sequence =
        rear_left_data->sample_sequence;

    s_safety_input.rear_right_distance_cm =
        rear_right_data->filtered_distance_cm;
    s_safety_input.rear_right_distance_valid =
        rear_right_data->valid;
    s_safety_input.rear_right_sample_sequence =
        rear_right_data->sample_sequence;
}

/* API CÔNG KHAI */

/* Khoi tao driver, bo loc va dang ky ba sonar. */
bool RA_Control_Init(void)
{
    uint8_t channel_index;

    /* Xóa dữ liệu cũ trước khi khởi tạo lại driver và bộ lọc. */
    for (channel_index = 0U;
         channel_index < (uint8_t)SONAR_COUNT;
         channel_index++)
    {
        RA_Control_ResetSonarChannel(
            &s_sonar_channels[channel_index]);
    }

    /*
     * RA_Control là nơi duy nhất đăng ký cảm biến của xe. Module debug chỉ
     * đọc lại dữ liệu công khai, không gọi Dev_Sonar_GetNewData() lần thứ hai.
     */
    Dev_Sonar_Init();

    s_sonar_channels[SONAR_FRONT].driver_id =
        Dev_Sonar_Add(
            SONAR_FRONT_TRIG_PORT,
            SONAR_FRONT_TRIG_PIN,
            SONAR_FRONT_ECHO_PORT,
            SONAR_FRONT_ECHO_PIN);

    s_sonar_channels[SONAR_REAR_L].driver_id =
        Dev_Sonar_Add(
            SONAR_REAR_LEFT_TRIG_PORT,
            SONAR_REAR_LEFT_TRIG_PIN,
            SONAR_REAR_LEFT_ECHO_PORT,
            SONAR_REAR_LEFT_ECHO_PIN);

    s_sonar_channels[SONAR_REAR_R].driver_id =
        Dev_Sonar_Add(
            SONAR_REAR_RIGHT_TRIG_PORT,
            SONAR_REAR_RIGHT_TRIG_PIN,
            SONAR_REAR_RIGHT_ECHO_PORT,
            SONAR_REAR_RIGHT_ECHO_PIN);

    /* Đứng im cũng dùng lịch ưu tiên trước để giữ đúng chu kỳ cảm biến sau. */
    RA_Control_ResetSonarScheduler();

    /* Đầu vào chuyển động mặc định là xe đứng im và encoder chưa có tốc độ. */
    s_safety_input.direction = RA_DIR_STOPPED;
    s_safety_input.left_speed_mps = 0.0f;
    s_safety_input.right_speed_mps = 0.0f;
    s_safety_input.encoder_valid = false;
    s_safety_input.commanded_speed_mps = 0.0f;
    s_safety_input.front_distance_cm = 0.0f;
    s_safety_input.front_distance_valid = false;
    s_safety_input.front_sample_sequence = 0UL;
    s_safety_input.rear_left_distance_cm = 0.0f;
    s_safety_input.rear_left_distance_valid = false;
    s_safety_input.rear_left_sample_sequence = 0UL;
    s_safety_input.rear_right_distance_cm = 0.0f;
    s_safety_input.rear_right_distance_valid = false;
    s_safety_input.rear_right_sample_sequence = 0UL;
    RA_Init(&s_safety_output);

    s_control_ready =
        (s_sonar_channels[SONAR_FRONT].driver_id !=
         DEV_SONAR_ID_INVALID) &&
        (s_sonar_channels[SONAR_REAR_L].driver_id !=
         DEV_SONAR_ID_INVALID) &&
        (s_sonar_channels[SONAR_REAR_R].driver_id !=
         DEV_SONAR_ID_INVALID);

    return s_control_ready;
}

/* Chay sonar scheduler va state machine khong blocking. */
void RA_Control_Process(void)
{
    uint8_t channel_index;
    bool channel_updated[SONAR_COUNT] = { false };

    /* Driver phải được chạy liên tục để phát Trigger và xử lý timeout Echo. */
    Dev_Sonar_Process();

    /* Mỗi cờ dữ liệu mới được lấy đúng một lần và đưa qua bộ lọc tương ứng. */
    for (channel_index = 0U;
         channel_index < (uint8_t)SONAR_COUNT;
         channel_index++)
    {
        channel_updated[channel_index] = RA_Control_UpdateSonarChannel(
            &s_sonar_channels[channel_index]);
    }

    /* Chỉ đổi slot sau khi đúng cảm biến hiện tại đã trả Echo hoặc timeout. */
    if ((s_safety_input.direction == RA_DIR_FORWARD) ||
        (s_safety_input.direction == RA_DIR_STOPPED) ||
        (s_safety_input.direction == RA_DIR_OTHER))
    {
        const SonarPosition_t completed_position =
            s_sonar_schedule[s_sonar_schedule_index];

        if (channel_updated[(uint32_t)completed_position])
        {
            s_sonar_schedule_index++;
            if (s_sonar_schedule_index >= SONAR_SCHEDULE_COUNT)
            {
                s_sonar_schedule_index = 0U;
            }

            Dev_Sonar_SetScanMask(
                RA_Control_GetPositionScanMask(
                    s_sonar_schedule[s_sonar_schedule_index]));
        }
    }

    /* Reverse Assist nhận cả ba khoảng cách theo cm và đã qua median filter. */
    RA_Control_UpdateSafetyInputFromSonar();
    RA_Process(&s_safety_input, &s_safety_output);
}

/* Cap nhat huong, encoder va toc do yeu cau cua xe. */
void RA_Control_SetMotionInput(
    RA_Direction_t direction,
    float left_speed_mps,
    float right_speed_mps,
    bool encoder_valid,
    float commanded_speed_mps)
{
    const RA_Direction_t previous_direction = s_safety_input.direction;

    s_safety_input.direction = direction;
    s_safety_input.left_speed_mps = left_speed_mps;
    s_safety_input.right_speed_mps = right_speed_mps;
    s_safety_input.encoder_valid = encoder_valid;
    s_safety_input.commanded_speed_mps = commanded_speed_mps;

    /* Cập nhật encoder cùng hướng không được reset lịch hoặc ghi lại mask. */
    if (direction == previous_direction)
    {
        return;
    }

    /*
     * Chỉ xóa mẫu lúc thật sự đổi hướng. Hàm có thể được gọi định kỳ để cập
     * nhật encoder mà không làm median filter bị khởi tạo lại liên tục.
     */
    if (direction != previous_direction)
    {
        if ((direction == RA_DIR_FORWARD) &&
            (previous_direction == RA_DIR_STOPPED))
        {
            /* Chi canh STOPPED -> FORWARD moi duoc xin nha AEB_HOLD. */
            RA_RequestForwardRestart();
            RA_Control_InvalidateSonarChannel(
                SONAR_FRONT);
        }
        else if (direction == RA_DIR_FORWARD)
        {
            RA_Control_InvalidateSonarChannel(
                SONAR_FRONT);
        }
        else if (direction == RA_DIR_REVERSE)
        {
            RA_Control_InvalidateSonarChannel(
                SONAR_REAR_L);
            RA_Control_InvalidateSonarChannel(
                SONAR_REAR_R);
        }
        else
        {
            /* STOPPED không cần xóa dữ liệu; chỉ đổi lại mặt nạ quét. */
        }
    }

    /* Tiến/đứng im dùng lịch ưu tiên; lùi giữ quét tuần tự hai cảm biến sau. */
    if (direction == RA_DIR_FORWARD)
    {
        RA_Control_ResetSonarScheduler();
    }
    else if (direction == RA_DIR_REVERSE)
    {
        Dev_Sonar_SetScanMask(RA_Control_GetRearScanMask());
    }
    else
    {
        RA_Control_ResetSonarScheduler();
    }
}

/* Doc snapshot cua mot vi tri sonar. */
bool RA_Control_GetSonarData(
    SonarPosition_t position,
    SonarData_t *data)
{
    if (((uint32_t)position >= (uint32_t)SONAR_COUNT) ||
        (data == NULL))
    {
        return false;
    }

    *data = s_sonar_channels[(uint32_t)position].public_data;
    return true;
}

/* Doc ket qua an toan moi nhat. */
bool RA_Control_GetSafetyOutput(RA_Output_t *output)
{
    if (output == NULL)
    {
        return false;
    }

    *output = s_safety_output;
    return true;
}

/* Kiem tra ba sonar da dang ky thanh cong. */
bool RA_Control_IsReady(void)
{
    return s_control_ready;
}
