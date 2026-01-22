#include "motor_app.h"
#include "my_socket.h"

tb6612_motor_t left_motor;
tb6612_motor_t right_motor;
motor_encoder_t left_encoder;
motor_encoder_t right_encoder;

#define TAG "motor_app"
#define PID_ALPHA 1.0f

PID_LocTypeDef left_motor_pid;
PID_LocTypeDef right_motor_pid;
extern QueueHandle_t cmd_queq;  //命令数据队列句柄
cmd_t current_cmd; //当前命令结构体

const float WHEEL_BASE = 0.125f;     // 轮间距 (米) 12.5cm间距
const float WHEEL_RADIUS = 0.024f;  // 轮半径 (米) 48mm直径轮

TickType_t last_rec_tick = 0; // 上一次接收命令的时间戳

void motor_app_init(void)
{
    ESP_LOGI(TAG, "Initializing motors...");
    //pid参数初始化
    PID_Init(&left_motor_pid, 1.15f, 1.1f, 0.1f, 1023.0f, -1023.0f);    //6.2 0.62
    PID_Init(&right_motor_pid, 1.15f, 1.1f, 0.1f, 1023.0f, -1023.0f);
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
    
    xTaskCreate(motor_speed_ring_task,"motor_speed_ring_task",2048,NULL,10,NULL);
    
}


void motor_speed_ring_task(void *pvParameters)
{
    xSemaphoreTake(key_semaphore, portMAX_DELAY);
    ESP_LOGI(TAG, "Key pressed! Starting motor speed control...");
    float target_speed_left = 0.0f;  // 目标速度，支持正负值
    float target_speed_right = 0.0f; // 目标速度，支持正负值
    while (1)
    {
        // 从命令队列获取最新的速度命令（非阻塞）
        if (xQueueReceive(cmd_queq, &current_cmd, 0) == pdTRUE) {
            double cmd_linear_x = current_cmd.linear_x;
            double cmd_angular_z = current_cmd.angular_z;
            last_rec_tick = xTaskGetTickCount(); // 更新接收命令的时间戳

            // 根据线速度和角速度计算左右轮目标速度
            // 步骤1：计算左右轮线速度 (m/s)
            //   v_left  = linear_x - angular_z × (WHEEL_BASE / 2)
            //   v_right = linear_x + angular_z × (WHEEL_BASE / 2)
            // 步骤2：线速度转RPM
            //   轮子周长 = 2π × WHEEL_RADIUS
            //   转速(转/秒) = 线速度 / 周长 = v / (2π × WHEEL_RADIUS)
            //   转速(RPM) = 转速(转/秒) × 60

            target_speed_left = (float)((cmd_linear_x - (cmd_angular_z * WHEEL_BASE / 2.0f)) / (2.0f * 3.14159f * WHEEL_RADIUS) * 60.0f);
            target_speed_right = (float)((cmd_linear_x + (cmd_angular_z * WHEEL_BASE / 2.0f)) / (2.0f * 3.14159f * WHEEL_RADIUS) * 60.0f);
        }
        if( (xTaskGetTickCount() - last_rec_tick) > pdMS_TO_TICKS(500) ) {
            // 超过500ms未收到新命令，停止电机
            target_speed_left = 0.0f;
            target_speed_right = 0.0f;
        }

        // PID 闭环控制（每个周期都执行，不管是否收到新命令）
        int32_t target_duty_left = PID_location(target_speed_left, left_encoder.rpm, &left_motor_pid);
        int32_t target_duty_right = PID_location(-target_speed_right, right_encoder.rpm, &right_motor_pid);

        // 输出到电机
        tb6612_motor_pwm_set(&left_motor, target_duty_left);
        tb6612_motor_pwm_set(&right_motor, target_duty_right);

        vTaskDelay(pdMS_TO_TICKS(50));
    }

}





