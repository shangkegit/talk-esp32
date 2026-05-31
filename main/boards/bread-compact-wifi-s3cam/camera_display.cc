#include "camera_display.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "display/lvgl_display/lvgl_image.h"
#include <cstring>

static const char *TAG = "CameraDisplay";

CameraDisplay::CameraDisplay(Esp32Camera *camera, LcdDisplay *display)
    : camera_(camera), display_(display), display_task_handle_(nullptr), is_running_(false), is_paused_(false)
{
    frame_mutex_ = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "CameraDisplay initialized");
}

CameraDisplay::~CameraDisplay()
{
    Stop();
    if (frame_mutex_) {
        vSemaphoreDelete(frame_mutex_);
    }
}

void CameraDisplay::DisplayTask(void *arg)
{
    CameraDisplay *instance = static_cast<CameraDisplay *>(arg);
    ESP_LOGI(TAG, "Display task started");

    while (instance->is_running_)
    {
        if (instance->is_paused_)
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // 尝试获取帧锁，拿不到就等（说明正在拍照）
        if (xSemaphoreTake(instance->frame_mutex_, pdMS_TO_TICKS(500)) != pdTRUE)
        {
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            xSemaphoreGive(instance->frame_mutex_);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        if (fb->format != PIXFORMAT_RGB565)
        {
            esp_camera_fb_return(fb);
            xSemaphoreGive(instance->frame_mutex_);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        int frame_width = fb->width;
        int frame_height = fb->height;
        size_t pixel_count = frame_width * frame_height;
        size_t data_size = pixel_count * 2;

        uint8_t *preview_data = (uint8_t *)heap_caps_malloc(data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!preview_data)
        {
            esp_camera_fb_return(fb);
            xSemaphoreGive(instance->frame_mutex_);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        uint16_t *src = (uint16_t *)fb->buf;
        uint16_t *dst = (uint16_t *)preview_data;
        for (size_t i = 0; i < pixel_count; i++)
        {
            dst[i] = __builtin_bswap16(src[i]);
        }

        // 立即释放帧缓冲
        esp_camera_fb_return(fb);
        // 释放帧锁
        xSemaphoreGive(instance->frame_mutex_);

        auto image = std::make_unique<LvglAllocatedImage>(
            preview_data, data_size, frame_width, frame_height,
            frame_width * 2, LV_COLOR_FORMAT_RGB565);

        instance->display_->SetPreviewImage(std::move(image));
        vTaskDelay(pdMS_TO_TICKS(66));
    }

    ESP_LOGI(TAG, "Display task stopped");
    vTaskDelete(nullptr);
}

bool CameraDisplay::Start()
{
    if (is_running_)
    {
        return true;
    }

    is_running_ = true;
    is_paused_ = false;

    BaseType_t ret = xTaskCreatePinnedToCore(
        DisplayTask, "camera_display", 8192, this, 5, &display_task_handle_, 1);

    if (ret != pdPASS)
    {
        is_running_ = false;
        display_task_handle_ = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "Camera display started");
    return true;
}

void CameraDisplay::Stop()
{
    if (!is_running_) return;
    is_running_ = false;
    if (display_task_handle_)
    {
        vTaskDelay(pdMS_TO_TICKS(200));
        display_task_handle_ = nullptr;
    }
}
