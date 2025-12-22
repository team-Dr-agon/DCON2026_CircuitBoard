#include "inc/http.hpp"
#include "inc/pwm.hpp"
#include "inc/get.hpp"

// Webサーバーオブジェクト（ポート80）
WebServer server(80);

// センサーへのポインタ（main.inoから参照を受け取る）
static MAX30105* pSensorMAX30105 = nullptr;
static Adafruit_BNO08x_RVC* pSensorBNO = nullptr;

/**
 * @brief センサーの参照を設定（setupHTTPの前に呼び出す必要がある）
 */
void initHTTPSensors(MAX30105& max30105, Adafruit_BNO08x_RVC& bno08x) {
    pSensorMAX30105 = &max30105;
    pSensorBNO = &bno08x;
}


/**
 * @brief ルートパス "/" へのGETリクエスト処理
 */
void handleRoot() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>ESP32-C3 Web API</title>";
    html += "<style>body{font-family:Arial;margin:20px;} button{padding:10px 20px;margin:5px;font-size:16px;}</style>";
    html += "</head><body>";
    html += "<h1>ESP32-C3 Web API</h1>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

/**
 * @brief PWM設定API "/api/pwm?value=XX"
 */
void handlePWMAPI() {
    if (server.hasArg("value")) {
        float value = server.arg("value").toFloat();
        
        // PWM設定
        setPWM(value);        
        String json = "{\"status\":\"ok\",\"value\":" + String(value) + "}";
        server.send(200, "application/json", json);
    } else {
        String json = "{\"status\":\"error\",\"message\":\"value parameter required\"}";
        server.send(400, "application/json", json);
    }
}

/**
 * @brief ステータス取得API "/api/status"
 */
void handleStatusAPI() {
    String json = "{";
    json += "\"connected_clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"ssid\":\"" + String(AP_SSID) + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

/**
 * @brief センサーデータ取得API "/api/sensors"
 * 
 * センサーデータをJSON形式で返す例
 */
void handleSensorAPI() {
    HeartRateData hrData;
    IMUData imuData;
    
    // センサーデータ取得
    getHeartRateData(hrData);
    getIMUData(imuData);
    
    // 心拍数
    int heartRate = hrData.hrValid ? hrData.heartRate : -1;
    
    // SpO2
    int spo2 = hrData.spo2Valid ? hrData.spo2 : -1;
    
    // 温度
    float temperature = hrData.temperature;
    
    // 姿勢データ
    float Pitch = imuData.pitch;
    float Yaw = imuData.yaw;
    float Roll = imuData.roll;

    // JSON形式で組み立て
    String json = "{";
    json += "\"timestamp\":" + String(millis()) + ",";
    json += "\"heart_rate\":" + String(heartRate) + ",";
    json += "\"spo2\":" + String(spo2) + ",";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"imu_valid\":" + String(imuDataValid ? "true" : "false") + ",";
    json += "\"Pitch\":" + String(Pitch, 2) + ",";
    json += "\"Yaw\":" + String(Yaw, 2) + ",";
    json += "\"Roll\":" + String(Roll, 2);
    json += "}";
    server.send(200, "application/json", json);
}

/**
 * @brief 特定センサーのデータ取得 "/api/sensor/heartrate"
 */
void handleHeartRateAPI() {
    HeartRateData data;
    getHeartRateData(data);
    
    int heartRate = data.hrValid ? data.heartRate : -1;
    int spo2 = data.spo2Valid ? data.spo2 : -1;
    
    String json = "{";
    json += "\"sensor\":\"heartrate\",";
    json += "\"hr_valid\":" + String(data.hrValid ? "true" : "false") + ",";
    json += "\"spo2_valid\":" + String(data.spo2Valid ? "true" : "false") + ",";
    json += "\"heart_rate\":" + String(heartRate) + ",";
    json += "\"spo2\":" + String(spo2) + ",";
    json += "\"temperature\":" + String(data.temperature, 1) + ",";
    json += "\"ir_value\":" + String(data.irValue) + ",";
    json += "\"red_value\":" + String(data.redValue);
    json += "}";
    
    server.send(200, "application/json", json);
}

/**
 * @brief 加速度センサーのデータ取得 "/api/sensor/accel"
 */
void handleAccelAPI() {
    IMUData data;
    getIMUData(data);
    
    String json = "{";
    json += "\"sensor\":\"accelerometer\",";
    json += "\"valid\":" + String(imuDataValid ? "true" : "false") + ",";
    json += "\"Pitch\":" + String(data.pitch, 3) + ",";
    json += "\"Yaw\":" + String(data.yaw, 3) + ",";
    json += "\"Roll\":" + String(data.roll, 3) + ",";
    json += "\"x_accel\":" + String(data.x_accel, 3) + ",";
    json += "\"y_accel\":" + String(data.y_accel, 3) + ",";
    json += "\"z_accel\":" + String(data.z_accel, 3);
    json += "}";
    
    server.send(200, "application/json", json);
}

/**
 * @brief 404エラー処理
 */
void handleNotFound() {
    String json = "{\"status\":\"error\",\"message\":\"Not Found\"}";
    server.send(404, "application/json", json);
}

/**
 * @brief Wi-FiアクセスポイントとWebサーバーの初期化
 */
void setupHTTP() {
    // Wi-Fiアクセスポイント開始
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTIONS);
    
    delay(100); // APの安定化待ち
    
    IPAddress IP = WiFi.softAPIP();
    Serial.println("Wi-Fi AP起動");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("IP: ");
    Serial.println(IP);
    
    // ルーティング設定
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/pwm", HTTP_GET, handlePWMAPI);
    server.on("/api/status", HTTP_GET, handleStatusAPI);
    server.on("/api/sensors", HTTP_GET, handleSensorAPI);           // 全センサーデータ
    server.on("/api/sensor/heartrate", HTTP_GET, handleHeartRateAPI); // 心拍数
    server.on("/api/sensor/accel", HTTP_GET, handleAccelAPI);        // 加速度
    server.onNotFound(handleNotFound);
    
    // Webサーバー開始
    server.begin();
    Serial.println("Webサーバー起動完了");
}

/**
 * @brief Webサーバーのリクエスト処理
 *        loop()内で定期的に呼び出す
 */
void handleHTTP() {
    server.handleClient();
}

/**
 * @brief 接続中のクライアント数を取得
 */
int getConnectedClients() {
    return WiFi.softAPgetStationNum();
}