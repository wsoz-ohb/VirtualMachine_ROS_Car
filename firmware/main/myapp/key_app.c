#include "key_app.h"

static const char *TAG = "key_app";

uint8_t key_val,key_down,key_up,key_old;
uint8_t key_count=0;
SemaphoreHandle_t key_semaphore;
void key_app_init(void)
{
    key_init();//初始化按键
    key_semaphore=xSemaphoreCreateBinary();//创建按键信号量
    if (key_semaphore == NULL)
    {
        ESP_LOGE(TAG, "Failed to create key semaphore");
        configASSERT(key_semaphore);
    }
    xTaskCreate(key_app_task,"key_app_task",1024,NULL,10,NULL);//创建按键任务
}

void key_app_task(void *pvParameters)
{
    
    while(1)
    {
        key_val=key_read();
        key_down=(key_val) & (key_old ^ key_val);//按下事件
        key_up=(~key_val) & (key_old ^ key_val);//抬起事件
        key_old=key_val;
        
        if(key_down==1)
        {
            key_count++;
            //按键按下事件
        }
        if (key_count==1)
        {
            xSemaphoreGive(key_semaphore);//释放信号量
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

}
