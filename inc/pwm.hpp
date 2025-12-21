#ifndef PWM_HPP
#define PWM_HPP

#include <Arduino.h>

// PWM設定
#define PWM_PIN 2          // PWMピン番号（回路図に合わせて変更してください）
#define PWM_CHANNEL 0       // PWMチャンネル (0-15)
#define PWM_FREQ 5000       // PWM周波数 (Hz)
#define PWM_RESOLUTION 8    // PWM解像度 (8bit = 0-255)

// PWM初期化関数
void setupPWM();

// PWM制御関数（0-100%で指定）
void setPWM(float percentage);

#endif // PWM_HPP
