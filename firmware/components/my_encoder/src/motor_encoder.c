#include "motor_encoder.h"

static const char *TAG = "MOTOR_ENCODER";

// 全局编码器指针数组，支持两个编码器（左轮和右轮）
static motor_encoder_t *g_encoders[2] = {NULL, NULL};
static esp_timer_handle_t g_speed_timer = NULL;
static const float ENCODER_SAMPLE_PERIOD_S = 0.05f;  // 50 ms
static const float SECONDS_PER_MINUTE = 60.0f;
#define ENCODER_SAMPLE_PERIOD_US 50000

// 计算速度脉冲
static int16_t calculate_speed_pulse(motor_encoder_t* encoder)
{
    int current_count = 0;
    esp_err_t ret = pcnt_unit_get_count(encoder->pcnt_unit, &current_count);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "pcnt_unit_get_count failed: %d", ret);
        return 0;
    }

    int16_t speed_pulse = (int16_t)(current_count - encoder->last_count);
    encoder->last_count = current_count;

    return speed_pulse;
}

static void update_encoder_metrics(motor_encoder_t* encoder)
{
    encoder->speed_pulse = calculate_speed_pulse(encoder);
    encoder->total_pulse += encoder->speed_pulse;

    const float pulses_per_rev = encoder->multiply_factor * encoder->gear_ratio * encoder->ppr;
    if (pulses_per_rev > 0.0f) {
        encoder->rpm = ((float)encoder->speed_pulse) * (SECONDS_PER_MINUTE / ENCODER_SAMPLE_PERIOD_S) / pulses_per_rev;
    } else {
        encoder->rpm = 0.0f;
    }
}

// 50ms 定时器回调函数
void motor_encoder_speed_update_callback(void* arg)
{
    if (g_encoders[0] != NULL) {
        update_encoder_metrics(g_encoders[0]);
    }

    if (g_encoders[1] != NULL) {
        update_encoder_metrics(g_encoders[1]);
    }
}

// 编码器初始化
esp_err_t motor_encoder_init(motor_encoder_t* encoder, int unit_id,
                            uint8_t gpio_a, uint8_t gpio_b)
{
    if (encoder == NULL) {
        ESP_LOGE(TAG, "Encoder pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (unit_id < 0 || unit_id > 1) {
        ESP_LOGE(TAG, "Invalid unit_id: %d (must be 0 or 1)", unit_id);
        return ESP_ERR_INVALID_ARG;
    }

    encoder->encoder_a_gpio = gpio_a;
    encoder->encoder_b_gpio = gpio_b;
    encoder->unit_id = unit_id;
    encoder->ppr = 13.0f;
    encoder->gear_ratio = 20.0f;
    encoder->multiply_factor = 4.0f;
    encoder->total_pulse = 0;
    encoder->speed_pulse = 0;
    encoder->rpm = 0.0f;
    encoder->last_count = 0;
    encoder->is_initialized = false;

    pcnt_unit_config_t unit_config = {
        .high_limit = 32767,
        .low_limit = -32768,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &encoder->pcnt_unit));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = gpio_a,
        .level_gpio_num = gpio_b,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(encoder->pcnt_unit, &chan_a_config, &pcnt_chan_a));

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = gpio_b,
        .level_gpio_num = gpio_a,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(encoder->pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_enable(encoder->pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoder->pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(encoder->pcnt_unit));

    g_encoders[unit_id] = encoder;

    if (g_speed_timer == NULL) {
        esp_timer_create_args_t timer_args = {
            .callback = motor_encoder_speed_update_callback,
            .name = "encoder_speed_timer"
        };
        ESP_ERROR_CHECK(esp_timer_create(&timer_args, &g_speed_timer));
        ESP_ERROR_CHECK(esp_timer_start_periodic(g_speed_timer, ENCODER_SAMPLE_PERIOD_US));
    }

    encoder->is_initialized = true;
    ESP_LOGI(TAG, "Motor encoder initialized successfully (Unit: %d, GPIO_A: %d, GPIO_B: %d, PPR: %.0f, Gear: %.0f)",
             unit_id, gpio_a, gpio_b, encoder->ppr, encoder->gear_ratio);

    return ESP_OK;
}

// 编码器反初始化
esp_err_t motor_encoder_deinit(motor_encoder_t* encoder)
{
    if (encoder == NULL || !encoder->is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    pcnt_unit_stop(encoder->pcnt_unit);
    pcnt_unit_disable(encoder->pcnt_unit);
    pcnt_del_unit(encoder->pcnt_unit);

    if (g_encoders[encoder->unit_id] == encoder) {
        g_encoders[encoder->unit_id] = NULL;
    }

    if (g_encoders[0] == NULL && g_encoders[1] == NULL && g_speed_timer != NULL) {
        ESP_ERROR_CHECK(esp_timer_stop(g_speed_timer));
        ESP_ERROR_CHECK(esp_timer_delete(g_speed_timer));
        g_speed_timer = NULL;
    }

    encoder->is_initialized = false;
    ESP_LOGI(TAG, "Motor encoder deinitialized");

    return ESP_OK;
}

// 重置编码器计数
esp_err_t motor_encoder_reset(motor_encoder_t* encoder)
{
    if (encoder == NULL || !encoder->is_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    encoder->total_pulse = 0;
    encoder->speed_pulse = 0;
    encoder->rpm = 0.0f;
    encoder->last_count = 0;

    ESP_ERROR_CHECK(pcnt_unit_clear_count(encoder->pcnt_unit));

    ESP_LOGD(TAG, "Motor encoder reset");
    return ESP_OK;
}

// 获取当前速度脉冲
int16_t motor_encoder_get_speed_pulse(motor_encoder_t* encoder)
{
    if (encoder == NULL || !encoder->is_initialized) {
        return 0;
    }

    return encoder->speed_pulse;
}

// 获取总脉冲数
int32_t motor_encoder_get_total_pulse(motor_encoder_t* encoder)
{
    if (encoder == NULL || !encoder->is_initialized) {
        return 0;
    }

    return encoder->total_pulse;
}

// 获取转过的圈数
float motor_encoder_get_circle(motor_encoder_t* encoder)
{
    if (encoder == NULL || !encoder->is_initialized) {
        return 0.0f;
    }

    return (float)encoder->total_pulse / (encoder->multiply_factor * encoder->gear_ratio * encoder->ppr);
}
