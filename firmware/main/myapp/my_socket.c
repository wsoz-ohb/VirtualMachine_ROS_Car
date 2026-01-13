#include "my_socket.h"
#include "cJSON.h"

#define IP_TARGET "192.168.2.9"  // 虚拟机IP地址
#define PORT 8080       // imu目标端口
#define PORT2 8081      // lidar目标端口2
#define PORT3 8082      // 编码器目标端口3

static const char *TAG = "my_socket";
struct sockaddr_in server_addr;
struct sockaddr_in server_addr2;
struct sockaddr_in server_addr3;
static int sock = -1; // UDP套接字
struct rt_ringbuffer rec_ringbuffer; // 接收缓冲区
rt_uint8_t recv_buf[1024];
socket_state_t my_socket_stat = SOCKET_DISCONNECTED;

SemaphoreHandle_t socket_ok_to_send = NULL; // 发送信号量
SemaphoreHandle_t socket_ok_to_rec = NULL; // 接收信号量

int my_socket_init(void)
{
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 创建UDP套接字
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }

    // 初始化服务器地址结构
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_TARGET, &server_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid server address");
        close(sock);
        sock = -1;
        return -1;
    }
    ESP_LOGI(TAG, "UDP socket1 created successfully");


    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // 创建UDP套接字
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return -1;
    }

    // 初始化服务器地址结构
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_port = htons(PORT2);
    if (inet_pton(AF_INET, IP_TARGET, &server_addr2.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid server address");
        close(sock);
        sock = -1;
        return -1;
    }
    ESP_LOGI(TAG, "UDP socket2 created successfully");

    // 初始化服务器地址结构
    server_addr3.sin_family = AF_INET;
    server_addr3.sin_port = htons(PORT3);
    if (inet_pton(AF_INET, IP_TARGET, &server_addr3.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid server address");
        close(sock);
        sock = -1;
        return -1;
    }
    my_socket_stat = SOCKET_READY;
    ESP_LOGI(TAG, "UDP socket3 created successfully");
    return sock;
}

void socket_init_thread(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket thread waiting for Wi-Fi...");
    // 发送线程需要三路同时放行，用计数信号量解锁三个发送任务
    socket_ok_to_send = xSemaphoreCreateCounting(3, 0);
    if(socket_ok_to_send == NULL)
    {
        ESP_LOGE(TAG, "Failed to create socket_ok_to_send semaphore");
        vTaskDelete(NULL);
        return;
    }
    socket_ok_to_rec = xSemaphoreCreateBinary();
    if(socket_ok_to_rec == NULL)
    {
        ESP_LOGE(TAG, "Failed to create socket_ok_to_rec semaphore");
        vTaskDelete(NULL);
        return;
    }

    xSemaphoreTake(wifi_ok, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi is ready, initializing UDP socket...");

    while ((sock = my_socket_init()) < 0)
    {
        ESP_LOGI(TAG, "Socket init failed, retrying...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "UDP socket is ready!");
    xSemaphoreGive(socket_ok_to_send);
    xSemaphoreGive(socket_ok_to_send);
    xSemaphoreGive(socket_ok_to_send);
    xSemaphoreGive(socket_ok_to_rec);
    vTaskDelete(NULL);
}

void socket_send_thread(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket_send thread waiting for socket...");
    xSemaphoreTake(socket_ok_to_send, portMAX_DELAY);

    while (1)
    {
        if(my_socket_stat == SOCKET_READY)
        {
            //imu数据发送
            float temp_data[10];
            xQueueReceive(imu_queue, temp_data, portMAX_DELAY);//从队列接收数据imu
            //构建json数据
            // {
            // "type": "imu_data",
            // "data": {
            // "quat_i": 0.123456,
            // "quat_j": -0.054321,
            // "quat_k": 1.570796,
            // "quat_real": 0.123456,
            // "acc_x": -0.054321,
            // "acc_y": 1.570796,
            // "acc_z": 0.123456,
            // "gyro_x": -0.054321,
            // "gyro_y": 1.570796,
            // "gyro_z": 0.123456
            // }
            // }
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "imu_data");
            cJSON *data = cJSON_CreateObject();
            cJSON_AddNumberToObject(data, "quat_i", temp_data[0]);
            cJSON_AddNumberToObject(data, "quat_j", temp_data[1]);
            cJSON_AddNumberToObject(data, "quat_k", temp_data[2]);
            cJSON_AddNumberToObject(data, "quat_real", temp_data[3]);
            cJSON_AddNumberToObject(data, "acc_x", temp_data[4]);
            cJSON_AddNumberToObject(data, "acc_y", temp_data[5]);
            cJSON_AddNumberToObject(data, "acc_z", temp_data[6]);
            cJSON_AddNumberToObject(data, "gyro_x", temp_data[7]);
            cJSON_AddNumberToObject(data, "gyro_y", temp_data[8]);
            cJSON_AddNumberToObject(data, "gyro_z", temp_data[9]);
            cJSON_AddItemToObject(root, "data", data);
            char *json_string = cJSON_PrintUnformatted(root); // 将JSON对象转换为字符串

            // 发送imu数据
            int bytes_sent = sendto(sock, json_string, strlen(json_string), 0,
                                    (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (bytes_sent < 0)
            {
                ESP_LOGE(TAG, "Failed to send data: errno %d", errno);
            }

            // 释放内存
            cJSON_free(json_string);  // 释放JSON字符串内存
            cJSON_Delete(root);  // 释放JSON对象


        }
        vTaskDelay(50 / portTICK_PERIOD_MS);  //50ms发送一次
    }
}

void socket_send_thread2(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket_send thread2 waiting for socket...");
    xSemaphoreTake(socket_ok_to_send, portMAX_DELAY);

    while (1)
    {
        //lidar数据发送
        lidar_packet_t lidar_data;

        // 非阻塞接收，避免队列满
        if (xQueueReceive(lidar_queue, &lidar_data, pdMS_TO_TICKS(10)) == pdTRUE) {
            if(my_socket_stat == SOCKET_READY) {
                int data_sent = sendto(sock, lidar_data.data, lidar_data.length, 0,
                                        (struct sockaddr *)&server_addr2, sizeof(server_addr2));
            if (data_sent < 0)
            {
                ESP_LOGE(TAG, "Failed to send data: errno %d", errno);
            }
            } else {
                // WiFi未连接，丢弃数据
                ESP_LOGD(TAG, "Socket not ready, dropping lidar packet");
            }
        }

        vTaskDelay(5 / portTICK_PERIOD_MS);  // 5ms检查一次，快速消费队列
    }
}


void socket_send_thread3(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket_send thread3 waiting for socket...");
    xSemaphoreTake(socket_ok_to_send, portMAX_DELAY);

    while (1)
    {
        if(my_socket_stat == SOCKET_READY) {
            //编码器数据发送
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "odometry");
            cJSON_AddNumberToObject(root, "left_rpm", left_encoder.rpm);
            cJSON_AddNumberToObject(root, "right_rpm", right_encoder.rpm);
            char *json_string = cJSON_PrintUnformatted(root);

            int data_sent = sendto(sock, json_string, strlen(json_string), 0,
                            (struct sockaddr *)&server_addr3, sizeof(server_addr3));
            if (data_sent < 0)
            {
                ESP_LOGE(TAG, "Failed to send encoder data: errno %d", errno);
            }

            // 释放内存
            cJSON_free(json_string);
            cJSON_Delete(root);
        } else {
            // WiFi未连接，丢弃数据
            ESP_LOGD(TAG, "Socket not ready, dropping encoder data");
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);  // 50ms发送一次编码器数据
    }
}

void socket_rec_thread(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket_rec thread waiting for socket...");
    xSemaphoreTake(socket_ok_to_rec, portMAX_DELAY);

    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    while (1)
    {
        if(my_socket_stat == SOCKET_READY)
        {
            char rec_temp[1024];
            memset(rec_temp, 0, sizeof(rec_temp));

            // UDP接收数据
            int bytes_received = recvfrom(sock, rec_temp, sizeof(rec_temp) - 1, 0,
                                         (struct sockaddr *)&source_addr, &socklen);
            if (bytes_received > 0)
            {
                rec_temp[bytes_received] = '\0';

                // 将接收到的数据放入环形缓冲区
                rt_size_t put_len = rt_ringbuffer_put(&rec_ringbuffer,
                                                    (const rt_uint8_t*)rec_temp,
                                                    bytes_received);

                ESP_LOGI(TAG, "Received %d bytes from %s:%d",
                        bytes_received,
                        inet_ntoa(source_addr.sin_addr),
                        ntohs(source_addr.sin_port));

                // 检查缓冲区中是否有数据需要处理
                rt_size_t data_size = rt_ringbuffer_data_len(&rec_ringbuffer);
                if (data_size > 0)
                {
                    char *rec_op = (char*)malloc(data_size + 1);
                    if (rec_op != NULL)
                    {
                        memset(rec_op, 0, data_size + 1);
                        rt_size_t get_len = rt_ringbuffer_get(&rec_ringbuffer,
                                                            (rt_uint8_t*)rec_op,
                                                            data_size);
                        rec_op[get_len] = '\0';

                        ESP_LOGI(TAG, "Received: %s", rec_op);
                        free(rec_op);
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Memory allocation failed");
                    }
                }
            }
            else if (bytes_received < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    ESP_LOGD(TAG, "recvfrom timeout");
                } else {
                    ESP_LOGE(TAG, "recvfrom error: errno %d", errno);
                }
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void my_socket_start(void)
{
    // 创建环形缓冲区
    rt_ringbuffer_init(&rec_ringbuffer, recv_buf, sizeof(recv_buf));

    // 创建初始化线程
    xTaskCreate(socket_init_thread, "socket_init", 4096, NULL, 6, NULL);

    // 创建发送线程
    xTaskCreate(socket_send_thread, "socket_send", 4096, NULL, 4, NULL);
    xTaskCreate(socket_send_thread2, "socket_send2", 4096, NULL, 4, NULL);
    xTaskCreate(socket_send_thread3, "socket_send3", 4096, NULL, 4, NULL);

    // 创建接收线程
    xTaskCreate(socket_rec_thread, "socket_rec", 4096, NULL, 4, NULL);
}



