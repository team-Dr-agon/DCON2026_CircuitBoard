#ifndef GET_HPP
#define GET_HPP

#include <Arduino.h>
#include "MAX30105.h"
#include "Adafruit_BNO08x_RVC.h"
#include "heartRate.h"
#include "spo2_algorithm.h"


// センサーデータ構造体
struct HeartRateData {
    int32_t heartRate;      // 心拍数 (BPM)
    int32_t spo2;           // 酸素飽和度 (%)
    bool hrValid;           // 心拍数の有効性
    bool spo2Valid;         // SpO2の有効性
    uint32_t irValue;       // IR値
    uint32_t redValue;      // Red値
    float temperature;    // 体温 (℃)
};

struct IMUData {
    float yaw;              // ヨー角 (度)
    float pitch;            // ピッチ角 (度)
    float roll;             // ロール角 (度)
    float x_accel;          // X軸加速度 (m/s^2)
    float y_accel;          // Y軸加速度 (m/s^2)
    float z_accel;          // Z軸加速度 (m/s^2)
};

// センサー初期化関数
bool initMAX30105(MAX30105 &sensor);
bool initBNO08x(Adafruit_BNO08x_RVC &sensor, Stream *serial);

// データ更新関数（loop()で常に呼び出す）
void updateHeartRateBuffer(MAX30105 &sensor);

// データ取得関数（現在の値を返す）
void getHeartRateData(HeartRateData &data);
bool getIMUData(Adafruit_BNO08x_RVC &sensor, IMUData &data);

// ヘルパー関数
void printHeartRateData(const HeartRateData &data);
void printIMUData(const IMUData &data);

#endif // GET_HPP
