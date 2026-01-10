#include "inc/pwm.hpp"

// 現在のPWM値を保持するグローバル変数
static float currentPWM = 0.0;

/**
 * @brief PWMの初期化
 * 
 * PWMチャンネルをセットアップし、ピンに割り当てます。
 */
void setupPWM() {
    // ESP32 Arduino 3.x系の新しいAPIを使用
    ledcAttach(PWM_PIN, PWM_FREQ, PWM_RESOLUTION);
    ledcWrite(PWM_PIN, 0);
}

/**
 * @brief PWM出力を設定
 * 
 * @param percentage 0~100の範囲で調光レベルを指定
 *                   0% = 完全にOFF
 *                   100% = 最大出力
 */
void setPWM(float percentage) {
    // 範囲チェック
    if (percentage < 0.0) {
        percentage = 0.0;
    } else if (percentage > 100.0) {
        percentage = 100.0;
    }
    
    // 0-100%を0-255の範囲に変換（8bit解像度）
    int dutyCycle = (int)((percentage / 100.0) * 255.0);    
    // PWM出力
    ledcWrite(PWM_PIN, dutyCycle);
    
    // 現在値を保存
    currentPWM = percentage;
}

/**
 * @brief PWM現在値取得
 * 
 * @return 現在のPWM値（0-100%）
 */
float getPWM() {
    return currentPWM;
}
