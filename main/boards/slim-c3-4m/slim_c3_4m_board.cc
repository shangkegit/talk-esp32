#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "application.h"
#include "button.h"
#include "led/single_led.h"
#include "config.h"

#include <esp_log.h>
#include <esp_efuse_table.h>

#define TAG "SlimC3Board"

class SlimC3Board : public WifiBoard {
private:
    Button boot_button_;

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
        });
        boot_button_.OnPressDown([this]() {
            Application::GetInstance().StartListening();
        });
        boot_button_.OnPressUp([this]() {
            Application::GetInstance().StopListening();
        });
    }

public:
    SlimC3Board() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeButtons();

        // Use VDD SPI pins as GPIO on ESP32-C3
        esp_efuse_write_field_bit(ESP_EFUSE_VDD_SPI_AS_GPIO);
    }

    virtual AudioCodec* GetAudioCodec() override {
        // MAX98357 (speaker) + INMP441 (mic) - separate I2S buses
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            SPK_I2S_GPIO_BCLK, SPK_I2S_GPIO_WS, SPK_I2S_GPIO_DOUT,
            MIC_I2S_GPIO_SCK, MIC_I2S_GPIO_WS, MIC_I2S_GPIO_DIN
        );
        return &audio_codec;
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }
};

DECLARE_BOARD(SlimC3Board);
