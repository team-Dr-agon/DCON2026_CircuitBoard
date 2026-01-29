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

// FreeRTOSタスクハンドル
TaskHandle_t imuTaskHandle = NULL;

// データ更新間隔（ミリ秒）
#define SENSOR_UPDATE_INTERVAL 1000
unsigned long lastSensorUpdate = 0;

// HTTP処理を間引きするためのカウンタ
unsigned int loopCounter = 0;

// IMUデータ取得タスク（高優先度で実行）
void imuUpdateTask(void* parameter) {
    while (1) {
        updateIMUBuffer(bno08x, &BNO_SERIAL);
        vTaskDelay(1);  // 1ms待機（CPUを解放）
    }
}

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
    
    // BNO08x UARTを先に初期化して十分な待機時間を確保
    Serial.println("BNO08x Serial initializing...");
    BNO_SERIAL.begin(115200, SERIAL_8N1, BNO_RX, BNO_TX);
    BNO_SERIAL.setRxBufferSize(512);  // RXバッファサイズを増やす
    delay(500);  // 待機時間を増やす
    
    // バッファをクリア
    while (BNO_SERIAL.available()) {
        BNO_SERIAL.read();
    }
    Serial.println("BNO08x Serial initialized");
    
    initMAX30105(max30105);
    Serial.println("✓ MAX30105 ready");
    initBNO08x(bno08x, &BNO_SERIAL);
    Serial.println("✓ BNO08x ready");
    
    xTaskCreatePinnedToCore(
        imuUpdateTask,      // タスク関数
        "IMU_Task",          // タスク名
        4096,               // スタックサイズ
        NULL,               // パラメータ
        2,                  // 優先度（高め）
        &imuTaskHandle,     // タスクハンドル
        0                   // Core 0で実行
    );
    Serial.println("✓ IMU task started");
    
    initHTTPSensors(max30105, bno08x);
    setupHTTP();
    Serial.println("✓ HTTP server ready");
    
    Serial.println("\n=== Setup Complete ===\n");
    setPWM(0);  // 起動時はPWMを0%に設定    
}

void loop() {
    updateHeartRateBuffer(max30105);
    
    // HTTP処理は10回に1回のみ実行（センサー更新を優先）
    handleHTTP();

    
    // センサー診断
    if (millis() - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
        lastSensorUpdate = millis();
        
        // MAX30105の生データ確認
        Serial.print("[診断] IR値: ");
        Serial.print(max30105.getIR());
        Serial.print(", RED値: ");
        Serial.println(max30105.getRed());
        
        // BNO08xデータ確認
        IMUData imuData;
        getIMUData(imuData);
        
        // UARTバッファの状況を確認
        int availableBytes = BNO_SERIAL.available();
        Serial.print("[診断] UART Buffer: ");
        Serial.print(availableBytes);
        Serial.print(" bytes | ");
        
        if (imuDataValid) {
            Serial.print("IMU - Pitch: ");
            Serial.print(imuData.pitch, 2);
            Serial.print("°, Yaw: ");
            Serial.print(imuData.yaw, 2);
            Serial.print("° (Age: ");
            Serial.print(millis() - lastIMUUpdate);
            Serial.println("ms)");
        } else {
            Serial.println("IMU - データ無効（1秒以上更新なし）");
        }
        Serial.println();
    }
}
