#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include "config.h"
#include "camera.h"
#include "engine.h"
#include "mymqtt.h"

// 全局上下文
static mqtt_ctx g_mqtt_ctx;
volatile static int g_running = 1;

// 信号处理函数
void sigint_handler(int sig) {
    printf("\n收到终止信号，清理资源...\n");
    g_running = 0;
}

// 视频发布线程函数
void* video_publish_thread(void* arg) {
    (void)arg;
    // 暂时未设计
}

// 测试图像是否能完整发送到6818_image主题
void* test_data_publish(void* arg) {
    unsigned char* buffer = NULL;
    long size = 0;
    get_image_data(&buffer, &size);
    if (buffer && size > 0) {
        printf("[TEST] 成功读取图像数据，大小: %ld 字节，准备发送到6818_image主题\n", size);
        if (mqtt_publish(&g_mqtt_ctx, "6818_image", buffer, size) == 0) {
            printf("[TEST] 成功通过MQTT发送到6818_image主题\n");
        } else {
            printf("[TEST] 通过MQTT发送失败\n");
        }
        free(buffer);
    } else {
        printf("[TEST] 读取图像数据失败\n");
    }
}

int main() {
    // 注册信号处理
    signal(SIGINT, sigint_handler);

    // // 创建视频发布线程
    // pthread_t video_tid;
    // if(pthread_create(&video_tid, NULL, video_publish_thread, NULL) != 0) {
    //     fprintf(stderr, "视频发布线程创建失败\n");
    //     return 1;
    // }

    // 初始化MQTT
    if(mqtt_init(&g_mqtt_ctx, parse_json_and_control) != 0) {
        fprintf(stderr, "MQTT初始化失败\n");
        return 1;
    }
    printf("MQTT连接成功，已订阅主题: %s\n", TOPIC_SUB);
    test_data_publish(NULL); // 测试图像数据发布

    // pthread_join(video_tid, NULL);
    while (g_running) {
        sleep(1);
    }
    
    // 清理资源
    mqtt_disconnect(&g_mqtt_ctx);
    // engine_close();
    return 0;
}