#include "motor_app.h"

tb6612_motor_t left_motor;
tb6612_motor_t right_motor;
motor_encoder_t left_encoder;
motor_encoder_t right_encoder;

#define TAG "motor_app"
#define PID_ALPHA 1.0f

PID_LocTypeDef left_motor_pid;
PID_LocTypeDef right_motor_pid;

void motor_app_init(void)
{
    ESP_LOGI(TAG, "Initializing motors...");
    //pid参数初始化
    PID_Init(&left_motor_pid, 6.2f, 0.62f, 0.0f, 1023.0f, -1023.0f);    //5.8 0.62
    PID_Init(&right_motor_pid, 6.2f, 0.62f, 0.0f, 1023.0f, -1023.0f);

    // 左电机配置
    left_motor.dir_mode = TB6612_MOTOR_DIR_UNNORMAL;
    left_motor.pwm_channel = LEDC_CHANNEL_0;
    left_motor.tb6612_in1_gpio = 35; // GPIO35
    left_motor.tb6612_in2_gpio = 36; // GPIO36
    left_motor.tb6612_pwm_gpio = 4;  // GPIO4
    tb6612_gpio_register(&left_motor);
    ESP_LOGI(TAG, "Left motor registered");
    
    // 右电机配置
    right_motor.dir_mode = TB6612_MOTOR_DIR_UNNORMAL;
    right_motor.pwm_channel = LEDC_CHANNEL_1;
    right_motor.tb6612_in1_gpio = 47; // GPIO47
    right_motor.tb6612_in2_gpio = 48; // GPIO48
    right_motor.tb6612_pwm_gpio = 5;  // GPIO5
    tb6612_gpio_register(&right_motor);
    ESP_LOGI(TAG, "Right motor registered");

    // 编码器配置
    ESP_LOGI(TAG, "Initializing encoders...");
    ESP_ERROR_CHECK(motor_encoder_init(&left_encoder, 0, 6, 7));
    ESP_ERROR_CHECK(motor_encoder_init(&right_encoder, 1, 15, 16));
    ESP_LOGI(TAG, "Encoders initialized (left: GPIO6/7, right: GPIO15/16)");
    
    // // Start motors at ~68% duty
    // tb6612_motor_pwm_set(&left_motor, MOTOR_START_DUTY);
    // tb6612_motor_pwm_set(&right_motor, MOTOR_START_DUTY);

    // ESP_LOGI(TAG, "Motors started with PWM duty: %d", MOTOR_START_DUTY);
    xTaskCreate(motor_speed_ring_task,"motor_speed_ring_task",2048,NULL,10,NULL);
    
}


void motor_speed_ring_task(void *pvParameters)
{
    xSemaphoreTake(key_semaphore, portMAX_DELAY);
    ESP_LOGI(TAG, "Key pressed! Starting motor speed control...");
    uint16_t target_speed_left = 0;  // 目标速度，可以根据需要调整
    uint16_t target_speed_right = 0; // 目标速度，可以根据需要调整
    while (1) 
    {
        switch (key_count)
        {
        case 1:
            //ESP_LOGI(TAG, "Key count is 1");
            target_speed_left = 60;  // 目标速度，可以根据需要调整
            target_speed_right = 60; // 目标速度，可以根据需要调整
            break;
        
        case 2:
            //ESP_LOGI(TAG, "Key count is 2");
            target_speed_left = 120;  // 目标速度，可以根据需要调整
            target_speed_right = 120; // 目标速度，可以根据需要调整
            break;

        case 3:
            //ESP_LOGI(TAG, "Key count is 3");
            target_speed_left = 0;  // 目标速度，可以根据需要调整
            target_speed_right = 0; // 目标速度，可以根据需要调整
            break;

        default:
            break;
        }
        //ESP_LOGI(TAG, "Target speeds : %d, Readly speeds : %f", target_speed_left, left_encoder.rpm);
        float left_pid_out = PID_location(target_speed_left,left_encoder.rpm, &left_motor_pid);
        float right_pid_out = PID_location(target_speed_right,right_encoder.rpm, &right_motor_pid);

        int32_t left_out = (int32_t)(left_pid_out * PID_ALPHA);
        int32_t right_out = (int32_t)(right_pid_out * PID_ALPHA);
        //ESP_LOGI(TAG, "PID outputs : %f, %f", left_pid_out, right_pid_out);

        tb6612_motor_pwm_set(&left_motor, left_out);
        tb6612_motor_pwm_set(&right_motor, right_out);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

}





