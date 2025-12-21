#include "inc/get.hpp"

// バッファサイズ
#define BUFFER_LENGTH 100
#define SAMPLE_RATE 100  // Hz

// MAX30105センサーの初期化（成功するまでリトライ）
bool initMAX30105(MAX30105 &sensor) {
    Serial.println("↓ MAX30105センサーを初期化中...");
    
    while (!sensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println("MAX30105 not found. Retrying in 2 seconds...");
        delay(2000);
    }
    
    // センサー設定
    byte ledBrightness = 60;   // LED輝度 (0=Off to 255=50mA)
    byte sampleAverage = 4;    // サンプル平均化
    byte ledMode = 2;          // 2 = Red + IR
    byte sampleRate = 100;     // サンプルレート (samples/sec)
    int pulseWidth = 411;      // パルス幅 (μs)
    int adcRange = 4096;       // ADC範囲
    
    sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    
    Serial.println("MAX30105 initialized successfully");
    return true;
}

// BNO08xセンサーの初期化（成功するまでリトライ）
bool initBNO08x(Adafruit_BNO08x_RVC &sensor, Stream *serial) {
    Serial.println("↓ BNO08xセンサーを初期化中...");
    
    while (!sensor.begin(serial)) {
        Serial.println("BNO08x not found. Retrying in 2 seconds...");
        delay(2000);
    }
    
    Serial.println("BNO08x initialized successfully");
    return true;
}

// グローバルバッファ（静的データ保持用）
static uint32_t irBuffer[BUFFER_LENGTH];
static uint32_t redBuffer[BUFFER_LENGTH];
static int bufferIndex = 0;
static bool bufferReady = false;

// 最新のセンサーデータ（グローバル変数）
static HeartRateData currentHeartRateData = {0};

// センサーバッファを連続的に更新（loop()で常に呼び出す）
void updateHeartRateBuffer(MAX30105 &sensor) {
    // データの読み取り
    currentHeartRateData.irValue = sensor.getIR();
    currentHeartRateData.redValue = sensor.getRed();
    
    // バッファに保存（初回BUFFER_LENGTH個まで）
    if (bufferIndex < BUFFER_LENGTH) {
        irBuffer[bufferIndex] = currentHeartRateData.irValue;
        redBuffer[bufferIndex] = currentHeartRateData.redValue;
        bufferIndex++;
        
        if (bufferIndex >= BUFFER_LENGTH) {
            bufferReady = true;
        }
        
        // バッファが満たされるまでは無効データ
        currentHeartRateData.hrValid = false;
        currentHeartRateData.spo2Valid = false;
        return;
    }
    
    // バッファをシフト
    for (int i = 0; i < BUFFER_LENGTH - 1; i++) {
        irBuffer[i] = irBuffer[i + 1];
        redBuffer[i] = redBuffer[i + 1];
    }
    irBuffer[BUFFER_LENGTH - 1] = currentHeartRateData.irValue;
    redBuffer[BUFFER_LENGTH - 1] = currentHeartRateData.redValue;
    
    // 心拍数とSpO2の計算
    int8_t validSPO2;
    int8_t validHeartRate;
    
    maxim_heart_rate_and_oxygen_saturation(
        irBuffer, BUFFER_LENGTH,
        redBuffer,
        &currentHeartRateData.spo2, &validSPO2,
        &currentHeartRateData.heartRate, &validHeartRate
    );
    
    currentHeartRateData.spo2Valid = (validSPO2 != 0);
    currentHeartRateData.hrValid = (validHeartRate != 0);
    
    // 指が検出されているか確認（IR値が一定以上）
    if (currentHeartRateData.irValue < 50000) {
        currentHeartRateData.hrValid = false;
        currentHeartRateData.spo2Valid = false;
    }
    
    // 温度読み取り
    currentHeartRateData.temperature = sensor.readTemperature();
}

// 現在の心拍数・SpO2データを取得
void getHeartRateData(HeartRateData &data) {
    data = currentHeartRateData;
}

// IMUデータの取得
bool getIMUData(Adafruit_BNO08x_RVC &sensor, IMUData &data) {
    BNO08x_RVC_Data rvcData;
    
    if (!sensor.read(&rvcData)) {
        return false;
    }
    
    data.yaw = rvcData.yaw;
    data.pitch = rvcData.pitch;
    data.roll = rvcData.roll;
    data.x_accel = rvcData.x_accel;
    data.y_accel = rvcData.y_accel;
    data.z_accel = rvcData.z_accel;
    
    return true;
}

// 心拍数データの表示
void printHeartRateData(const HeartRateData &data) {
    Serial.print("IR=");
    Serial.print(data.irValue);
    Serial.print(", RED=");
    Serial.print(data.redValue);
    
    if (data.irValue < 50000) {
        Serial.println(" - No finger detected");
        return;
    }
    
    Serial.print(" | HR=");
    if (data.hrValid) {
        Serial.print(data.heartRate);
        Serial.print(" bpm");
    } else {
        Serial.print("--");
    }
    
    Serial.print(", SpO2=");
    if (data.spo2Valid) {
        Serial.print(data.spo2);
        Serial.print("%");
    } else {
        Serial.print("--");
    }
    
    Serial.println();
}

// IMUデータの表示
void printIMUData(const IMUData &data) {
    Serial.print("Yaw: ");
    Serial.print(data.yaw, 2);
    Serial.print("°, Pitch: ");
    Serial.print(data.pitch, 2);
    Serial.print("°, Roll: ");
    Serial.print(data.roll, 2);
    Serial.print("° | Accel X: ");
    Serial.print(data.x_accel, 3);
    Serial.print(" m/s², Y: ");
    Serial.print(data.y_accel, 3);
    Serial.print(" m/s², Z: ");
    Serial.print(data.z_accel, 3);
    Serial.println(" m/s²");
}
