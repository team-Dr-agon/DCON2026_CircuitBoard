/*
 * ESP32-C3 メインプログラム
 * DCON2026 プロジェクト用
 * 
 * 機能:
 * - MAX30105センサーで心拍数・SpO2測定
 * - BNO08xセンサーでIMU（姿勢・加速度）測定
 * - Wi-Fiアクセスポイント起動
 * - WebAPIサーバー（PWM制御、センサーデータ取得）
 */

#include <Wire.h>
#include <MAX30105.h>
#include <Adafruit_BNO08x_RVC.h>
#include "inc/get.hpp"
#include "inc/http.hpp"
#include "inc/pwm.hpp"

// センサーオブジェクト
MAX30105 max30105;
Adafruit_BNO08x_RVC bno08x;

// シリアル通信設定（BNO08x用）
#define BNO_SERIAL Serial1
#define BNO_TX 21
#define BNO_RX 20

// データ更新間隔（ミリ秒）
#define SENSOR_UPDATE_INTERVAL 1000
unsigned long lastSensorUpdate = 0;

void setup() {
    // シリアルモニター初期化
    Serial.begin(115200);
    Serial.println("\n\n=== ESP32-C3 Starting ===");
    
    // I2C初期化（MAX30105用）
    // PWM初期化
    // UART初期化（BNO08x用）
    setupPWM();
    Serial.println("PWM initialized");
    setPWM(20);
    Wire.begin();
    Wire.setTimeout(1000);
    BNO_SERIAL.begin(115200, SERIAL_8N1, BNO_RX, BNO_TX);
    delay(100);
    Serial.println("BNO08x Serial initialized");
    initMAX30105(max30105);
    Serial.println("✓ MAX30105 ready");
    initBNO08x(bno08x, &BNO_SERIAL);
    Serial.println("✓ BNO08x ready");
    initHTTPSensors(max30105, bno08x);
    setupHTTP();
    Serial.println("✓ HTTP server ready");
    
    Serial.println("=== Setup Complete ===\n");
    setPWM(0);
}

void loop() {
    // 心拍数センサーバッファを常に更新（平均値計算のため）
    updateHeartRateBuffer(max30105);
    handleHTTP();
}
