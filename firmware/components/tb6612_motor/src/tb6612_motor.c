#include "tb6612_motor.h"
#include "esp_err.h"

// 静态变量，确保定时器只初始化一次
static bool timer_initialized = false;

void tb6612_gpio_register(tb6612_motor_t* motor)
{
    // 配置 IN1 引脚
    gpio_config_t gpio_in1 = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << motor->tb6612_in1_gpio),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_in1));

    // 配置 IN2 引脚
    gpio_config_t gpio_in2 = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << motor->tb6612_in2_gpio),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_in2));
    ESP_LOGI("TB6612", "IN2 GPIO config: OK");

    // 配置 PWM 引脚为输出模式
    gpio_config_t gpio_pwm = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << motor->tb6612_pwm_gpio),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    ESP_ERROR_CHECK(gpio_config(&gpio_pwm));
    ESP_LOGI("TB6612", "PWM GPIO config: OK");

    // PWM 定时器只初始化一次（避免重复配置冲突）
    if (!timer_initialized) {
        ledc_timer_config_t tb6612_pwm_timer = {
            .clk_cfg = LEDC_AUTO_CLK,
            .duty_resolution = LEDC_TIMER_10_BIT,
            .freq_hz = 20000,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .timer_num = LEDC_TIMER_0
        };
        ESP_ERROR_CHECK(ledc_timer_config(&tb6612_pwm_timer));
        timer_initialized = true;
    } else {
        ESP_LOGI("TB6612", "PWM Timer already initialized, skipping");
    }

    // 配置 PWM 通道
    ledc_channel_config_t tb6612_pwm = {
        .channel = motor->pwm_channel,
        .duty = 0,
        .gpio_num = motor->tb6612_pwm_gpio,
        .hpoint = 0,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&tb6612_pwm));
    ESP_LOGI("TB6612", "PWM Channel config: OK");

    // 启动 PWM 定时器输出
    ESP_ERROR_CHECK(ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0));
    ESP_LOGI("TB6612", "PWM Timer resumed");

    // 设置方向
    if (motor->dir_mode == TB6612_MOTOR_DIR_NORMAL) {
        gpio_set_level(motor->tb6612_in1_gpio, 1);
        gpio_set_level(motor->tb6612_in2_gpio, 0);
        ESP_LOGI("TB6612", "Direction: FORWARD (IN1=HIGH, IN2=LOW)");
    } else {
        gpio_set_level(motor->tb6612_in1_gpio, 0);
        gpio_set_level(motor->tb6612_in2_gpio, 1);
        ESP_LOGI("TB6612", "Direction: BACKWARD (IN1=LOW, IN2=HIGH)");
    }

    // 初始化为静止状态
    ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel);
}

static inline void tb6612_apply_direction(tb6612_motor_t *motor, bool reverse)
{
    int in1_level = 0;
    int in2_level = 0;

    if (motor->dir_mode == TB6612_MOTOR_DIR_NORMAL) {
        in1_level = reverse ? 0 : 1;
        in2_level = reverse ? 1 : 0;
    } else {
        in1_level = reverse ? 1 : 0;
        in2_level = reverse ? 0 : 1;
    }

    gpio_set_level(motor->tb6612_in1_gpio, in1_level);
    gpio_set_level(motor->tb6612_in2_gpio, in2_level);
}

void tb6612_motor_pwm_set(tb6612_motor_t* motor, int32_t duty)
{
    // 参数检查
    if (motor == NULL) {
        ESP_LOGE("TB6612", "Motor pointer is NULL");
        return;
    }

    bool reverse = duty < 0;
    uint32_t abs_duty = reverse ? (uint32_t)(-duty) : (uint32_t)duty;
    const uint32_t MAX_DUTY = 1023; // 2^10 - 1

    if (abs_duty > MAX_DUTY) {
        abs_duty = MAX_DUTY;
    }

    // 停止状态处理
    if (duty == 0) {
        gpio_set_level(motor->tb6612_in1_gpio, 0);
        gpio_set_level(motor->tb6612_in2_gpio, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel);
        return;
    }

    tb6612_apply_direction(motor, reverse);

    // 设置 PWM 占空比
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel, abs_duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, motor->pwm_channel));
}
