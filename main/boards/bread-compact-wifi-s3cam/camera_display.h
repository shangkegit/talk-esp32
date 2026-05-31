#pragma once

#include "esp32_camera.h"
#include "display/lcd_display.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>

// 前置声明，避免循环依赖
class Esp32Camera;
class LcdDisplay;

// 相机显示类 - 实时预览摄像头画面到 LCD 屏幕
class CameraDisplay {
private:
    Esp32Camera* camera_;
    LcdDisplay* display_;
    TaskHandle_t display_task_handle_;
    bool is_running_;

    // 任务函数
    static void DisplayTask(void* arg);

public:
    CameraDisplay(Esp32Camera* camera, LcdDisplay* display);
    ~CameraDisplay();

    // 启动显示任务
    bool Start();

    // 停止显示任务
    void Stop();

    // 获取运行状态
    bool IsRunning() const { return is_running_; }
};
