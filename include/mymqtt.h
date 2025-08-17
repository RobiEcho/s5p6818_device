#ifndef MYMQTT_H
#define MYMQTT_H

#include <stdio.h>
#include <stdint.h>
#include "config.h"
#include "MQTTClient.h"

// 回调函数类型定义
typedef void (*message_handler)(const char* payload);

// mqtt视频帧头部
typedef struct {
    uint32_t frame_id;// 帧ID
    uint32_t frame_len;// payload长度
} __attribute__((packed)) frame_header_t;//禁止编译器自动对齐

// MQTT 上下文结构体
typedef struct {
    MQTTClient client;
    message_handler handler;
    int connected;                // 连接状态标志
} mqtt_ctx;

// 初始化MQTT连接
int mqtt_init(mqtt_ctx* ctx, message_handler handler);

// 发布消息
int mqtt_publish(mqtt_ctx* ctx, const char* topic, 
                const void* payload, size_t payload_len);

// 断开连接并清理资源
void mqtt_disconnect(mqtt_ctx* ctx);

#endif /* MYMQTT_H */