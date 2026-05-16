#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// MAX98357 (Speaker output) - I2S bus 1
#define SPK_I2S_GPIO_BCLK GPIO_NUM_4
#define SPK_I2S_GPIO_WS   GPIO_NUM_5
#define SPK_I2S_GPIO_DOUT GPIO_NUM_3

// INMP441 (Microphone input) - I2S bus 2
#define MIC_I2S_GPIO_SCK GPIO_NUM_8
#define MIC_I2S_GPIO_WS  GPIO_NUM_7
#define MIC_I2S_GPIO_DIN GPIO_NUM_2

// Boot button (on-board)
#define BOOT_BUTTON_GPIO GPIO_NUM_9

// Status LED (D4 on Luat ESP32-C3 CORE board)
#define BUILTIN_LED_GPIO GPIO_NUM_12

#endif // _BOARD_CONFIG_H_
