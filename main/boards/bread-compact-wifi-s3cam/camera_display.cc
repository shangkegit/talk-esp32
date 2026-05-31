#include "camera_display.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "display/lvgl_display/lvgl_image.h"
#include <cstring>

static const char *TAG = "CameraDisplay";

// 构造函数
CameraDisplay::CameraDisplay(Esp32Camera *camera, LcdDisplay *display)
    : camera_(camera), display_(display), display_task_handle_(nullptr), is_running_(false)
{
    ESP_LOGI(TAG, "CameraDisplay initialized");
}

// 析构函数
CameraDisplay::~CameraDisplay()
{
    Stop();
}

// 显示任务 - 循环抓帧并显示到屏幕
void CameraDisplay::DisplayTask(void *arg)
{
    CameraDisplay *instance = static_cast<CameraDisplay *>(arg);
    ESP_LOGI(TAG, "Display task started");

    while (instance->is_running_)
    {
        // 1. 直接从摄像头获取一帧（使用 ESP-IDF API）
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            ESP_LOGE(TAG, "Failed to get camera frame");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 2. 检查帧格式（bread-compact-wifi-s3cam 配置为 RGB565）
        if (fb->format != PIXFORMAT_RGB565)
        {
            ESP_LOGW(TAG, "Frame format is not RGB565: %d", fb->format);
            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 3. 计算 RGB565 数据大小
        size_t data_size = fb->width * fb->height * 2; // RGB565 每像素 2 字节

        // 4. 分配预览数据缓冲区（使用 PSRAM）
        uint8_t *preview_data = (uint8_t *)heap_caps_malloc(data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!preview_data)
        {
            ESP_LOGE(TAG, "Failed to allocate preview buffer: %zu bytes", data_size);
            esp_camera_fb_return(fb);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // 5. 保存帧尺寸（释放后不再访问 fb）
        int frame_width = fb->width;
        int frame_height = fb->height;

        // 6. 复制帧数据到预览缓冲区，做 RGB565 字节交换
        size_t pixel_count = data_size / 2;
        uint16_t *src = (uint16_t *)fb->buf;
        uint16_t *dst = (uint16_t *)preview_data;
        for (size_t i = 0; i < pixel_count; i++) {
            dst[i] = __builtin_bswap16(src[i]);
        }

        // 7. 尽快释放帧缓冲
        esp_camera_fb_return(fb);

        // 8. 创建 LvglAllocatedImage 并显示
        auto image = std::make_unique<LvglAllocatedImage>(
            preview_data,
            data_size,
            frame_width,
            frame_height,
            frame_width * 2,  // stride = width * 2 for RGB565
            LV_COLOR_FORMAT_RGB565
        );

        instance->display_->SetPreviewImage(std::move(image));

        // 9. 控制帧率（约 15fps，避免 CPU 占用过高）
        vTaskDelay(pdMS_TO_TICKS(66));
    }

    ESP_LOGI(TAG, "Display task stopped");
    vTaskDelete(nullptr);
}

// 启动显示任务
bool CameraDisplay::Start()
{
    if (is_running_)
    {
        ESP_LOGW(TAG, "Display is already running");
        return true;
    }

    is_running_ = true;
    // 创建 FreeRTOS 任务（绑定到核心 1，避开 WiFi/蓝牙核心 0）
    BaseType_t ret = xTaskCreatePinnedToCore(
        DisplayTask,
        "camera_display",
        8192,  // 栈大小
        this,
        5,     // 任务优先级（中等）
        &display_task_handle_,
        1);    // 核心 1

    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create display task");
        is_running_ = false;
        display_task_handle_ = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "Camera display started successfully");
    return true;
}

// 停止显示任务
void CameraDisplay::Stop()
{
    if (!is_running_)
        return;

    is_running_ = false;

    if (display_task_handle_)
    {
        // 等待任务退出
        vTaskDelay(pdMS_TO_TICKS(200));
        display_task_handle_ = nullptr;
    }

    ESP_LOGI(TAG, "Camera display stopped");
}
