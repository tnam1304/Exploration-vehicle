/*
 * Cau hinh chung cho cac chuc nang Reverse Assist.
 * Cac gia tri can duoc hieu chinh lai tren xe that sau khi tich hop.
 */

#ifndef APP_RA_CONFIG_H
#define APP_RA_CONFIG_H

/* ======================= CƠ KHÍ VÀ ENCODER ======================= */

#define VEH_PI                    3.14159265f /* Hằng số pi. */
#define VEH_WHEEL_DIAMETER_M      0.065f      /* Đường kính bánh 6,5 cm. */
#define VEH_ENCODER_PPR           20.0f       /* Số xung mỗi vòng. */
#define VEH_MAX_SPEED_MPS         1.600f      /* Cần đo lại trên xe thật. */
#define VEH_ENCODER_MAX_SPEED_MPS 2.500f      /* Giới hạn phát hiện encoder lỗi. */

#define VEH_DISTANCE_PER_PULSE_M \
    (VEH_PI * VEH_WHEEL_DIAMETER_M / VEH_ENCODER_PPR)

/* Nhỏ hơn giá trị này được coi là xe đứng im. */
#define VEH_STOP_SPEED_MPS        0.050f

/* ======================= FORWARD SAFETY ========================== */

#define FS_SYSTEM_DELAY_S          0.120f /* Tre cua lich quet sonar. */
#define FS_FCW_TIME_S              0.300f /* Thoi gian canh bao som. */
#define FS_COMFORT_DECEL_MPS2      2.000f /* Giam toc em. */
#define FS_EMERGENCY_DECEL_MPS2    6.000f /* Giam toc khan cap. */
#define FS_DISTANCE_MARGIN_M       0.050f /* Khoang du phong. */
#define FS_MIN_PREBRAKE_MPS        0.200f /* Toc do nho nhat truoc AEB. */
#define FS_CONFIRM_SAMPLES         3U     /* So mau xac nhan trang thai. */
#define FS_HYSTERESIS_M            0.010f /* Chong dao dong khoang cach. */

/* Tien cham sau khi nguoi dung xin nha AEB_HOLD. */
#define FS_CREEP_SPEED_MPS         0.050f /* Toc do tien sat. */
#define FS_CREEP_STOP_DISTANCE_M   0.060f /* Dung khi con 6 cm. */

/* ======================= REVERSE WARNING ========================= */

#define RW_DANGER_DIST_M           0.050f /* Khoang cach nguy hiem. */
#define RW_WARNING_DIST_M          0.080f /* Khoang cach canh bao. */

/* ======================= REAR COLLISION ========================== */

#define RC_HISTORY_COUNT          4U      /* So mau tinh toc do. */
#define RC_SAMPLE_PERIOD_S        0.120f  /* Chu ky moi cam bien. */
#define RC_MONITOR_DIST_CM        150.0f  /* Vung theo doi. */
#define RC_MIN_VCLOSE_CM_S         15.0f  /* Loc nhieu toc do gan. */
#define RC_WARNING_DIST_CM         80.0f  /* Khoang cach canh bao. */
#define RC_DANGER_DIST_CM          40.0f  /* Khoang cach nguy hiem. */
#define RC_WARNING_TTC_S            2.0f  /* TTC canh bao. */
#define RC_DANGER_TTC_S             1.0f  /* TTC nguy hiem. */
#define RC_APPROACH_CONFIRM        2U     /* Xac nhan tien gan. */
#define RC_WARNING_CONFIRM         2U     /* Xac nhan canh bao. */
#define RC_DANGER_CONFIRM          1U     /* Xac nhan nguy hiem. */
#define RC_CLEAR_CONFIRM           3U     /* Xac nhan het nguy co. */

/* Tang toc ho tro khi co nguy co va cham tu phia sau. */
#define RC_BOOST_MIN_FRONT_CM     120.0f /* Khoang trong toi thieu. */
#define RC_BOOST_FRONT_MARGIN_CM   20.0f /* Khoang du so voi FCW. */
#define RC_BOOST_RATIO              1.15f /* Tang muc tieu 15 phan tram. */

/* ======================= BUZZER ================================== */

#define RA_BUZZER_SLOW_PERIOD_US  800000UL /* Chu ky canh bao cham. */
#define RA_BUZZER_SLOW_ON_US      150000UL /* Thoi gian bat nhip cham. */
#define RA_BUZZER_FAST_PERIOD_US  300000UL /* Chu ky canh bao nhanh. */
#define RA_BUZZER_FAST_ON_US      150000UL /* Thoi gian bat nhip nhanh. */

/* ======================= BO LOC VA DON VI ======================== */

#define RA_MEDIAN_SIZE            5U          /* Median cho từng sonar. */

#define M_PER_CM                  0.010f
#define CM_PER_M                  100.0f
#define MM_PER_M                  1000.0f

#if (RA_MEDIAN_SIZE == 0U)
#error "RA_MEDIAN_SIZE must be greater than zero"
#endif

#if ((FS_CONFIRM_SAMPLES == 0U) || (RC_HISTORY_COUNT < 2U))
#error "Invalid Reverse Assist sample count"
#endif

#if ((RC_APPROACH_CONFIRM == 0U) || \
     (RC_WARNING_CONFIRM == 0U) || \
     (RC_DANGER_CONFIRM == 0U) || \
     (RC_CLEAR_CONFIRM == 0U))
#error "Rear Collision confirm counts must be greater than zero"
#endif

#endif /* APP_RA_CONFIG_H */
