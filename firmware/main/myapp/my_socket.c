#include "my_socket.h"
#include "cJSON.h"
#include <stdbool.h>

#include "tb6612_motor.h"

#define IP_TARGET "192.168.2.9"  // 虚拟机IP地址
#define PORT 8080       // 传感器数据发送端口 (IMU + 编码器)
#define PORT2 8081      // LiDAR数据发送端口
#define PORT3 8082      // 控制命令接收端口

static const char *TAG = "my_socket";
struct sockaddr_in server_addr;
struct sockaddr_in server_addr2;
static int sock = -1;      // UDP发送套接字
static int recv_sock = -1; // UDP接收套接字 (绑定8082)
struct rt_ringbuffer rec_ringbuffer; // 接收缓冲区
rt_uint8_t recv_buf[1024];
socket_state_t my_socket_stat = SOCKET_DISCONNECTED;

SemaphoreHandle_t socket_ok_to_send = NULL; // 发送信号量
SemaphoreHandle_t socket_ok_to_rec = NULL; // 接收信号量

extern tb6612_motor_t left_motor;
extern tb6612_motor_t right_motor;

UBaseType_t cmd_uxQueueLength=10;  // 队列深度10
UBaseType_t cmd_uxItemSize=sizeof(cmd_t); //队列每个元素大小
QueueHandle_t cmd_queq=NULL; //命令数据队列句柄
cmd_t recv_cmd; //接收的命令数据

int my_socket_init(void)
{
    // 创建发送 socket（用于 8080 和 8081 两个端口）
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create send socket: errno %d", errno);
        return -1;
    }
    ESP_LOGI(TAG, "UDP send socket created successfully");

    // 配置第一个目标地址（8080 - IMU + 编码器）
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, IP_TARGET, &server_addr.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid server address for port %d", PORT);
        close(sock);
        sock = -1;
        return -1;
    }

    // 配置第二个目标地址（8081 - LiDAR）
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_port = htons(PORT2);
    if (inet_pton(AF_INET, IP_TARGET, &server_addr2.sin_addr) <= 0) {
        ESP_LOGE(TAG, "Invalid server address for port %d", PORT2);
        close(sock);
        sock = -1;
        return -1;
    }

    // 创建接收 socket 并绑定 8082 端口
    recv_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (recv_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create recv socket: errno %d", errno);
        close(sock);
        sock = -1;
        return -1;
    }

    struct sockaddr_in recv_addr;
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(PORT3);
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有接口

    if (bind(recv_sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0)
    {
        ESP_LOGE(TAG, "Failed to bind recv socket to port %d: errno %d", PORT3, errno);
        close(recv_sock);
        close(sock);
        recv_sock = -1;
        sock = -1;
        return -1;
    }
    ESP_LOGI(TAG, "UDP recv socket bound to port %d", PORT3);

    cmd_queq = xQueueCreate(cmd_uxQueueLength, cmd_uxItemSize);

    if (cmd_queq == NULL) {
        ESP_LOGE(TAG, "Failed to create cmd_queq");
        close(recv_sock);
        close(sock);
        recv_sock = -1;
        sock = -1;
        return -1;
    }

    my_socket_stat = SOCKET_READY;
    return sock;
}

void socket_init_thread(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket thread waiting for Wi-Fi...");
    // 发送线程需要两路同时放行，用计数信号量解锁两个发送任务
    socket_ok_to_send = xSemaphoreCreateCounting(2, 0);
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
    xSemaphoreGive(socket_ok_to_rec);
    vTaskDelete(NULL);
}

void socket_send_thread(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket_send thread waiting for socket...");
    xSemaphoreTake(socket_ok_to_send, portMAX_DELAY);

    while (1)
    {
        // 阻塞接收 IMU 数据（由生产者的 50ms 周期控制速率）
        float temp_data[10];
        xQueueReceive(imu_queue, temp_data, portMAX_DELAY);

        // 只在 socket ready 时发送，否则丢弃
        if(my_socket_stat == SOCKET_READY)
        {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "type", "sensor_data");

            // IMU 数据
            cJSON *imu = cJSON_CreateObject();
            cJSON_AddNumberToObject(imu, "quat_i", temp_data[0]);
            cJSON_AddNumberToObject(imu, "quat_j", temp_data[1]);
            cJSON_AddNumberToObject(imu, "quat_k", temp_data[2]);
            cJSON_AddNumberToObject(imu, "quat_real", temp_data[3]);
            cJSON_AddNumberToObject(imu, "acc_x", temp_data[4]);
            cJSON_AddNumberToObject(imu, "acc_y", temp_data[5]);
            cJSON_AddNumberToObject(imu, "acc_z", temp_data[6]);
            cJSON_AddNumberToObject(imu, "gyro_x", temp_data[7]);
            cJSON_AddNumberToObject(imu, "gyro_y", temp_data[8]);
            cJSON_AddNumberToObject(imu, "gyro_z", temp_data[9]);
            cJSON_AddItemToObject(root, "imu", imu);

            // 编码器数据
            cJSON *odometry = cJSON_CreateObject();
            cJSON_AddNumberToObject(odometry, "left_rpm", left_encoder.rpm);
            cJSON_AddNumberToObject(odometry, "right_rpm", right_encoder.rpm);
            cJSON_AddItemToObject(root, "odometry", odometry);

            char *json_string = cJSON_PrintUnformatted(root);

            int bytes_sent = sendto(sock, json_string, strlen(json_string), 0,
                                    (struct sockaddr *)&server_addr, sizeof(server_addr));
            if (bytes_sent < 0)
            {
                ESP_LOGE(TAG, "Failed to send sensor data: errno %d", errno);
            }

            cJSON_free(json_string);
            cJSON_Delete(root);
        }
        // 不需要延时，队列接收本身就控制了速率
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

void socket_rec_thread(void *pvParameters)
{
    ESP_LOGI(TAG, "Socket_rec thread waiting for socket...");
    xSemaphoreTake(socket_ok_to_rec, portMAX_DELAY);
    ESP_LOGI(TAG, "Socket_rec thread started, listening on port %d", PORT3);

    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    while (1)
    {
        if(my_socket_stat == SOCKET_READY)
        {
            char rec_temp[512];
            memset(rec_temp, 0, sizeof(rec_temp));

            // 从 8082 端口接收控制命令
            int bytes_received = recvfrom(recv_sock, rec_temp, sizeof(rec_temp) - 1, 0,
                                         (struct sockaddr *)&source_addr, &socklen);    

            if (bytes_received > 0)
            {
                rec_temp[bytes_received] = '\0';

                ESP_LOGI(TAG, "Received: %s", rec_temp);

                // 解析 JSON
                cJSON *root = cJSON_Parse(rec_temp);
                if (root == NULL)
                {
                    ESP_LOGW(TAG, "JSON parse failed");
                    continue;
                }
                // 命令格式示例
                // {
                //   "type": "cmd_vel",
                //   "linear_x": 0.5,
                //   "angular_z": 0.1
                // }
                // 提取三个字段
                cJSON *type_item = cJSON_GetObjectItem(root, "type");
                cJSON *linear_x_item = cJSON_GetObjectItem(root, "linear_x");   //线速度
                cJSON *angular_z_item = cJSON_GetObjectItem(root, "angular_z"); //角速度

                if (type_item != NULL && linear_x_item != NULL && angular_z_item != NULL)
                {
                    const char *type = type_item->valuestring;
                    float linear_x = (float)linear_x_item->valuedouble;
                    float angular_z = (float)angular_z_item->valuedouble;

                    ESP_LOGI(TAG, "Parsed: type=%s, linear_x=%.3f, angular_z=%.3f", type, linear_x, angular_z);

                   //队列传递
                    recv_cmd.linear_x = linear_x;
                    recv_cmd.angular_z = angular_z;
                    xQueueSend(cmd_queq, &recv_cmd, portMAX_DELAY);
                }
                else
                {
                    ESP_LOGW(TAG, "JSON missing required fields");
                }

                cJSON_Delete(root);
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

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
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

    // 创建接收线程
    xTaskCreate(socket_rec_thread, "socket_rec", 4096, NULL, 4, NULL);
}



