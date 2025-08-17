#include "mymqtt.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "engine.h"

// MQTT 消息送达回调函数
static void delivered(void *context, MQTTClient_deliveryToken dt) {
    // 消息送达处理
    printf("消息已发布: %d\n", dt);
}

// MQTT 消息收到回调函数
static int msgarrvd(mqtt_ctx* ctx, char *topicName, int topicLen, 
                   MQTTClient_message *message) {
    char* payload = NULL;// 用于开辟接收信息的内存

    if (message && message->payload && message->payloadlen > 0) {
        payload = malloc(message->payloadlen + 1);
        if (payload) {
            memcpy(payload, message->payload, message->payloadlen);
            payload[message->payloadlen] = '\0';
            printf("mqtt:收到消息: %s\n", payload);
            /* 后续可以调整为只处理来自TOPIC_SUB主题的数据 */

            // 消息来自TOPIC_SUB主题，则调用回调处理数据
            if(topicName && strcmp(topicName, TOPIC_SUB) == 0) {
                if(ctx->handler) {
                    ctx->handler(payload);// 调用舵机库的解析函数
                }
            }
        } else {
            fprintf(stderr, "mqtt:内存分配失败\n");
        }
    } else {
        fprintf(stderr, "mqtt:收到无效消息\n");
    }

    // 释放分配的内存，防止内存泄漏
    if (payload) free(payload);
    // 释放 MQTT 消息对象
    MQTTClient_freeMessage(&message);
    // 释放主题名字符串
    if (topicName) MQTTClient_free(topicName);
    // 返回 1 表示消息已处理
    return 1;
}

// MQTT 连接断开回调函数
static void connlost(mqtt_ctx* ctx, char *cause) {
    // 标记连接状态为断开，后续由主循环处理重连
    ctx->connected = 0;
    fprintf(stderr, "连接丢失，原因: %s\n", cause);    
}

// 初始化 MQTT 连接
int mqtt_init(mqtt_ctx* ctx, message_handler handler) {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    
    // 初始化上下文结构体
    memset(ctx, 0, sizeof(mqtt_ctx));
    ctx->handler = handler;      // 保存外部传入的消息处理函数
    ctx->connected = 0;          // 初始为未连接
    
    // 创建 MQTT 客户端实例
    if ((rc = MQTTClient_create(&ctx->client, DEFAULT_ADDRESS, DEFAULT_CLIENT_ID,
                              MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "创建客户端失败: %d\n", rc);
        return rc;
    }
    
    // 设置回调函数，包括连接丢失、消息到达、消息送达
    if ((rc = MQTTClient_setCallbacks(ctx->client, ctx, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "设置回调失败: %d\n", rc);
        MQTTClient_destroy(&ctx->client);
        return rc;
    }
    
    // 设置连接参数
    conn_opts.keepAliveInterval = 20; // 保活时间
    conn_opts.cleansession = 1;       // 清除会话
    conn_opts.connectTimeout = DEFAULT_TIMEOUT; // 连接超时时间
    
    // 建立与 MQTT 服务器的连接
    if ((rc = MQTTClient_connect(ctx->client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "连接失败: %d\n", rc);
        MQTTClient_destroy(&ctx->client);
        return rc;
    }
    
    // 订阅指定主题，确保能收到消息
    if ((rc = MQTTClient_subscribe(ctx->client, TOPIC_SUB, DEFAULT_QOS)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "订阅失败: %d\n", rc);
        MQTTClient_disconnect(ctx->client, DEFAULT_TIMEOUT);
        MQTTClient_destroy(&ctx->client);
        return rc;
    }
    
    ctx->connected = 1; // 标记为已连接
    return MQTTCLIENT_SUCCESS;
}

// 发布消息到指定主题
int mqtt_publish(mqtt_ctx* ctx, const char* topic, 
                const void* payload, size_t payload_len) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    
    // 参数检查，确保上下文、主题、载荷有效
    if (!ctx || !topic || !payload || payload_len == 0) {
        fprintf(stderr, "发布参数无效\n");
        return -1;
    }
    
    // 检查连接状态，未连接不能发布
    if (!ctx->connected) {
        fprintf(stderr, "MQTT未连接，无法发布\n");
        return -2;
    }
    
    // 设置消息内容
    pubmsg.payload = (void*)payload;
    pubmsg.payloadlen = (int)payload_len;
    pubmsg.qos = DEFAULT_QOS; // 服务质量
    pubmsg.retained = 0;      // 不保留消息
    
    // 发布消息
    rc = MQTTClient_publishMessage(ctx->client, topic, &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "发布失败: %d\n", rc);
        return rc;
    }
    
    // 等待消息送达服务器（可选，保证消息已发送）
    if ((rc = MQTTClient_waitForCompletion(ctx->client, token, DEFAULT_TIMEOUT)) != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "等待完成失败: %d\n", rc);
    }
    return rc;
}

// 断开 MQTT 连接并释放资源
void mqtt_disconnect(mqtt_ctx* ctx) {
    if (ctx && ctx->client) {
        ctx->connected = 0; // 标记为断开连接
        // 断开与服务器的连接
        MQTTClient_disconnect(ctx->client, DEFAULT_TIMEOUT);
        // 销毁客户端实例，释放资源
        MQTTClient_destroy(&ctx->client);
    }
}