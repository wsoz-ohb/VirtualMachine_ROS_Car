#include "bno08x_app.h"

static const char *TAG = "BNO08X_APP";
bno08x_data_t bno08x_data; //定义一个全局变量用于存储传感器数据
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          400000  // 400kHz
#define BNO08X_ADDR                 BNO080_DEFAULT_ADDRESS

uint8_t data_flash_flag=0;  //imu数据刷新标志位
UBaseType_t uxQueueLength=10;
UBaseType_t uxItemSize=sizeof(float)*3; //队列每个元素大小,存放三个float数据
QueueHandle_t imu_queue; //定义队列句柄

void get_data(void)
{
	if(dataAvailable())
	{
		bno08x_data.quat_i=getQuatI();
		bno08x_data.quat_j=getQuatJ();
		bno08x_data.quat_k=getQuatK();
		bno08x_data.quat_real=getQuatReal();
		
		bno08x_data.acc_x=getAccelX();
		bno08x_data.acc_y=getAccelY();
		bno08x_data.acc_z=getAccelZ();
		
		float temp1=0;
		float temp2=0;
		float temp3=0;
		QuaternionToEulerAngles(bno08x_data.quat_i,bno08x_data.quat_j,bno08x_data.quat_k,bno08x_data.quat_real,&temp1,&temp2,&temp3);
		bno08x_data.roll=temp1;
		bno08x_data.pitch=temp2;
		bno08x_data.yaw=temp3;

        data_flash_flag=1; //imu数据刷新完成
	}
}

void bno08x_app_task(void *pvParameters)
{
    while (1) {
        get_data();
        if(data_flash_flag==1)
        {
            data_flash_flag=0;
            //将数据放入队列
            float temp_data[3];
            temp_data[0]=bno08x_data.roll;
            temp_data[1]=bno08x_data.pitch;
            temp_data[2]=bno08x_data.yaw;
            xQueueSend(imu_queue, temp_data, portMAX_DELAY); 
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void bno08x_app_init(void)
{
    
    bno08x_data.scl_gpio=42;
    bno08x_data.sda_gpio=41;
    // 配置I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = bno08x_data.sda_gpio,
        .scl_io_num = bno08x_data.scl_gpio,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err == ESP_ERR_INVALID_STATE || err == ESP_FAIL) {
        ESP_LOGW(TAG, "I2C port %d already configured, reusing existing settings", I2C_MASTER_NUM);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(err));
        return;
    }
    
    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err == ESP_ERR_INVALID_STATE || err == ESP_FAIL) {
        ESP_LOGW(TAG, "I2C driver already installed on port %d, reusing existing driver", I2C_MASTER_NUM);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return;
    }
    
    ESP_LOGI(TAG, "I2C initialized successfully");
    
    // 初始化BNO08X
    BNO080_Init(I2C_MASTER_NUM, BNO08X_ADDR);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 软复位
    softReset();
    vTaskDelay(pdMS_TO_TICKS(300));
    
    // 启用旋转向量，100ms更新一次
    enableRotationVector(100);
    //启用加速度计，100ms更新一次
    enableAccelerometer(400);
    
    ESP_LOGI(TAG, "BNO08X initialized successfully");
    
    imu_queue=xQueueCreate(uxQueueLength, uxItemSize ); //队列创建

    // 创建任务
    xTaskCreate(bno08x_app_task, "bno08x_app_task", 4096, NULL, 10, NULL);
}




