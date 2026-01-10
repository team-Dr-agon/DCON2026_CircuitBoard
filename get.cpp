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
    
    // Part IDを確認して実際にセンサーが存在するか検証
    byte partID = sensor.readPartID();
    Serial.print("MAX30105 Part ID: 0x");
    Serial.println(partID, HEX);
    
    if (partID != 0x15) {  // MAX30105のPart IDは0x15
        Serial.println("警告: MAX30105が検出されていない可能性があります");
    }
    
    // センサー設定
    byte ledBrightness = 58;  // LED輝度 (0=Off to 255=50mA) - 検出向上のため増加
    byte sampleAverage = 8;    // サンプル平均化
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
    
    int retryCount = 0;
    const int maxRetries = 5;
    
    while (!sensor.begin(serial)) {
        Serial.print("BNO08x not found. Retry ");
        Serial.print(++retryCount);
        Serial.print("/");
        Serial.println(maxRetries);
        
        if (retryCount >= maxRetries) {
            Serial.println("ERROR: BNO08x initialization failed after max retries");
            Serial.println("Check: 1) Wiring, 2) TX/RX pins, 3) Power supply");
            return false;
        }
        
        // シリアルバッファをクリア
        while (serial->available()) {
            serial->read();
        }
        
        delay(1000);
    }
    
    Serial.println("BNO08x initialized successfully");
    
    // ウォームアップ: センサーが安定するまで待つ
    Serial.println("BNO08x warming up...");
    delay(200);  // センサーの安定化待ち
    
    // バッファに溜まった初期データのみクリア（待たずに読めるものだけ）
    BNO08x_RVC_Data dummyData;
    int discarded = 0;
    while (sensor.read(&dummyData) && discarded < 20) {
        discarded++;
        // delay無し: バッファにあるものだけ読む
    }
    Serial.print("Discarded ");
    Serial.print(discarded);
    Serial.println(" initial packets");
    
    return true;
}

// グローバルバッファ（静的データ保持用）
static uint32_t irBuffer[BUFFER_LENGTH];
static uint32_t redBuffer[BUFFER_LENGTH];
static int bufferIndex = 0;
static bool bufferReady = false;

// 最新のセンサーデータ(グローバル変数)
static HeartRateData currentHeartRateData = {0};
static IMUData currentIMUData = {0};
bool imuDataValid = false;
unsigned long lastIMUUpdate = 0;

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
    // LED輝度100の場合、5000以上あれば検出と判断
    if (currentHeartRateData.irValue < 5000) {
        currentHeartRateData.hrValid = false;
        currentHeartRateData.spo2Valid = false;
    }
    
    // 心拍数の妥当性チェック（40〜200 bpmの範囲外は無効化）
    // 安静時は通常60〜100 bpm、運動時でも最大200 bpm程度
    if (currentHeartRateData.heartRate < 40 || currentHeartRateData.heartRate > 200) {
        currentHeartRateData.hrValid = false;
    }
    
    // SpO2の妥当性チェック（70〜100%の範囲外は無効化）
    if (currentHeartRateData.spo2 < 70 || currentHeartRateData.spo2 > 100) {
        currentHeartRateData.spo2Valid = false;
    }
    
    // 温度読み取り
    currentHeartRateData.temperature = sensor.readTemperature();
}

// 現在の心拍数・SpO2データを取得
void getHeartRateData(HeartRateData &data) {
    data = currentHeartRateData;
}

// IMUバッファの更新（loop()で常に呼び出す）
void updateIMUBuffer(Adafruit_BNO08x_RVC &sensor, Stream *serial) {
    BNO08x_RVC_Data rvcData;
    
    // BNO085は100Hzでデータ出力（約10ms周期）
    // UARTバッファに溜まったデータを全てクリアして、最新のみ使う
    
    bool dataReceived = false;
    int readCount = 0;
    
    // バッファが空になるまで全て読む（最大100個まで安全装置）
    while (sensor.read(&rvcData) && readCount < 100) {
        currentIMUData.yaw = rvcData.yaw;
        currentIMUData.pitch = rvcData.pitch;
        currentIMUData.roll = rvcData.roll;
        currentIMUData.x_accel = rvcData.x_accel;
        currentIMUData.y_accel = rvcData.y_accel;
        currentIMUData.z_accel = rvcData.z_accel;
        dataReceived = true;
        readCount++;
    }
    
    // データが読めない状態が続き、バッファが溜まっている場合はリセット
    static unsigned long lastClearTime = 0;
    static int clearAttempts = 0;
    
    if (!dataReceived && millis() - lastIMUUpdate > 500) {
        // 1秒に1回だけクリア（連続クリアを防止）
        if (millis() - lastClearTime > 1000) {
            // UARTバッファの生データを直接クリア
            int cleared = 0;
            while (serial->available() && cleared < 512) {
                serial->read();
                cleared++;
            }
            
            clearAttempts++;
            Serial.print("[IMU] Buffer cleared (");
            Serial.print(cleared);
            Serial.print(" bytes) - attempt ");
            Serial.println(clearAttempts);
            
            // 3回クリアしても改善しない場合はセンサー再初期化
            if (clearAttempts >= 3) {
                Serial.println("[IMU] Re-initializing sensor...");
                delay(100);
                if (sensor.begin(serial)) {
                    Serial.println("[IMU] Re-initialization successful");
                    clearAttempts = 0;
                } else {
                    Serial.println("[IMU] Re-initialization failed");
                }
            }
            
            lastClearTime = millis();
        }
    } else if (dataReceived) {
        // データ取得成功時はカウンタリセット
        clearAttempts = 0;
    }
    
    if (dataReceived) {
        imuDataValid = true;
        lastIMUUpdate = millis();
    }
    
    // 50ms以上更新がない場合は無効化（100Hzなので5パケット分）
    if (millis() - lastIMUUpdate > 50) {
        imuDataValid = false;
    }
}

// 現在のIMUデータを取得（キャッシュから）
void getIMUData(IMUData &data) {
    data = currentIMUData;
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
