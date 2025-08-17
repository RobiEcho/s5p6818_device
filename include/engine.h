#ifndef ENGINE_H
#define ENGINE_H

#include <stdio.h>
#include <stdlib.h>
#include "config.h"

// 舵机编号定义
#define Engine2 0x1
#define Engine3 0x2

// 全局变量声明
extern double eng2_deg;
extern double eng3_deg;

void handle_angle_control(const char *json_data);
int engine_init();
void print_engine_angle();
void parse_json_and_control(const char *json_data);
void reset_engine();
void control_engine(int command, double *angle, double new_angle);
void engine_close();

#endif /* ENGINE_H */