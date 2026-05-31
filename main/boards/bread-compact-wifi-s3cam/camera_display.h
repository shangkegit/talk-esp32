#pragma once

#include "esp32_camera.h"
#include "display/lcd_display.h"
#include "esp_camera.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <atomic>
#include "freertos/semphr.h"

class Esp32Camera;
class LcdDisplay;

class CameraDisplay {
private:
    Esp32Camera* camera_;
    LcdDisplay* display_;
    TaskHandle_t display_task_handle_;
    std::atomic<bool> is_running_;
    std::atomic<bool> is_paused_;
    SemaphoreHandle_t frame_mutex_;  // 帧互斥锁，拍照和取景互斥

    static void DisplayTask(void* arg);

public:
    CameraDisplay(Esp32Camera* camera, LcdDisplay* display);
    ~CameraDisplay();

    bool Start();
    void Stop();

    // 暂停/恢复取景
    void Pause() { is_paused_ = true; }
    void Resume() { is_paused_ = false; }

    // 获取帧互斥锁（拍照时使用）
    SemaphoreHandle_t GetFrameMutex() { return frame_mutex_; }

    bool IsRunning() const { return is_running_; }
};
