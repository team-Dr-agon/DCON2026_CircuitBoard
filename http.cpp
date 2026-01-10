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
    html += "<title>ESP32-C3 Sensor Monitor</title>";
    html += "<style>";
    html += "body{font-family:Arial;margin:20px;background:#f5f5f5;}";
    html += ".container{max-width:1200px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
    html += "h1{color:#333;text-align:center;}";
    html += "h2{color:#555;border-bottom:2px solid #ddd;padding-bottom:10px;margin-top:30px;}";
    html += ".status{display:flex;justify-content:space-around;margin:20px 0;flex-wrap:wrap;}";
    html += ".status-item{background:#f0f0f0;padding:15px;border-radius:5px;margin:5px;min-width:120px;text-align:center;}";
    html += ".status-item .label{font-size:12px;color:#666;}";
    html += ".status-item .value{font-size:24px;font-weight:bold;color:#333;margin-top:5px;}";
    html += ".valid{color:#4CAF50;}.invalid{color:#f44336;}";
    html += ".chart-box{margin:20px 0;padding:10px;background:#fafafa;border-radius:5px;}";
    html += ".chart-title{font-weight:bold;margin-bottom:10px;color:#555;}";
    html += "canvas{width:100%;height:200px;border:1px solid #ddd;background:white;}";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>ESP32-C3 Sensor Monitor</h1>";
    html += "<h2>Heart Rate & SpO2</h2>";
    html += "<div class='status'>";
    html += "<div class='status-item'><div class='label'>Heart Rate</div><div class='value' id='hr'>--</div></div>";
    html += "<div class='status-item'><div class='label'>SpO2</div><div class='value' id='spo2'>--</div></div>";
    html += "<div class='status-item'><div class='label'>Temperature</div><div class='value' id='temp'>--</div></div>";
    html += "<div class='status-item'><div class='label'>IR Value</div><div class='value' id='ir'>--</div></div>";
    html += "</div>";
    html += "<div class='chart-box'><div class='chart-title'>Heart Rate (bpm)</div><canvas id='hrChart'></canvas></div>";
    html += "<div class='chart-box'><div class='chart-title'>SpO2 (%)</div><canvas id='spo2Chart'></canvas></div>";
    html += "<h2>9-Axis IMU</h2>";
    html += "<div class='status'>";
    html += "<div class='status-item'><div class='label'>Yaw</div><div class='value' id='yaw'>--</div></div>";
    html += "<div class='status-item'><div class='label'>Pitch</div><div class='value' id='pitch'>--</div></div>";
    html += "<div class='status-item'><div class='label'>Roll</div><div class='value' id='roll'>--</div></div>";
    html += "</div>";
    html += "<div class='chart-box'><div class='chart-title'>Orientation (degrees)</div><canvas id='orientChart'></canvas></div>";
    html += "<div class='chart-box'><div class='chart-title'>Acceleration X (m/s²)</div><canvas id='accelXChart'></canvas></div>";
    html += "<div class='chart-box'><div class='chart-title'>Acceleration Y (m/s²)</div><canvas id='accelYChart'></canvas></div>";
    html += "<div class='chart-box'><div class='chart-title'>Acceleration Z (m/s²)</div><canvas id='accelZChart'></canvas></div>";
    html += "<h2>PWM Output</h2>";
    html += "<div class='status'>";
    html += "<div class='status-item'><div class='label'>PWM Value</div><div class='value' id='pwm'>--</div></div>";
    html += "</div>";
    html += "<div class='chart-box'><div class='chart-title'>PWM Output (%)</div><canvas id='pwmChart'></canvas></div>";
    html += "</div>";
    html += "<script>";
    html += "const maxPoints=50;";
    html += "const hrData=[];const spo2Data=[];";
    html += "const yawData=[];const pitchData=[];const rollData=[];";
    html += "const accelXData=[];const accelYData=[];const accelZData=[];";
    html += "const pwmData=[];";
    html += "const hrCanvas=document.getElementById('hrChart');";
    html += "const spo2Canvas=document.getElementById('spo2Chart');";
    html += "const orientCanvas=document.getElementById('orientChart');";
    html += "const accelXCanvas=document.getElementById('accelXChart');";
    html += "const accelYCanvas=document.getElementById('accelYChart');";
    html += "const accelZCanvas=document.getElementById('accelZChart');";
    html += "const pwmCanvas=document.getElementById('pwmChart');";
    html += "const hrCtx=hrCanvas.getContext('2d');";
    html += "const spo2Ctx=spo2Canvas.getContext('2d');";
    html += "const orientCtx=orientCanvas.getContext('2d');";
    html += "const accelXCtx=accelXCanvas.getContext('2d');";
    html += "const accelYCtx=accelYCanvas.getContext('2d');";
    html += "const accelZCtx=accelZCanvas.getContext('2d');";
    html += "const pwmCtx=pwmCanvas.getContext('2d');";
    html += "function drawChart(ctx,data,min,max,color){";
    html += "const w=ctx.canvas.width;const h=ctx.canvas.height;";
    html += "ctx.clearRect(0,0,w,h);";
    html += "ctx.fillStyle='#fff';ctx.fillRect(0,0,w,h);";
    html += "ctx.strokeStyle='#ddd';ctx.lineWidth=1;";
    html += "for(let i=0;i<=4;i++){const y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}";
    html += "if(data.length<2)return;";
    html += "const step=w/(maxPoints-1);";
    html += "const range=max-min;";
    html += "ctx.strokeStyle=color;ctx.lineWidth=2;ctx.beginPath();";
    html += "for(let i=0;i<data.length;i++){";
    html += "const x=(maxPoints-data.length+i)*step;";
    html += "const y=h-(data[i]-min)/range*h;";
    html += "if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}";
    html += "ctx.stroke();}";
    html += "function drawMultiChart(ctx,datasets,min,max){";
    html += "const w=ctx.canvas.width;const h=ctx.canvas.height;";
    html += "ctx.clearRect(0,0,w,h);";
    html += "ctx.fillStyle='#fff';ctx.fillRect(0,0,w,h);";
    html += "ctx.strokeStyle='#ddd';ctx.lineWidth=1;";
    html += "for(let i=0;i<=4;i++){const y=h*i/4;ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}";
    html += "const step=w/(maxPoints-1);";
    html += "const range=max-min;";
    html += "datasets.forEach((dataset,idx)=>{";
    html += "if(dataset.data.length<2)return;";
    html += "ctx.strokeStyle=dataset.color;ctx.lineWidth=2;ctx.beginPath();";
    html += "for(let i=0;i<dataset.data.length;i++){";
    html += "const x=(maxPoints-dataset.data.length+i)*step;";
    html += "const y=h-(dataset.data[i]-min)/range*h;";
    html += "if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);}";
    html += "ctx.stroke();});}";
    html += "function resizeCanvas(){";
    html += "hrCanvas.width=hrCanvas.offsetWidth;hrCanvas.height=200;";
    html += "spo2Canvas.width=spo2Canvas.offsetWidth;spo2Canvas.height=200;";
    html += "orientCanvas.width=orientCanvas.offsetWidth;orientCanvas.height=200;";
    html += "accelXCanvas.width=accelXCanvas.offsetWidth;accelXCanvas.height=200;";
    html += "accelYCanvas.width=accelYCanvas.offsetWidth;accelYCanvas.height=200;";
    html += "accelZCanvas.width=accelZCanvas.offsetWidth;accelZCanvas.height=200;";
    html += "pwmCanvas.width=pwmCanvas.offsetWidth;pwmCanvas.height=200;";
    html += "drawChart(hrCtx,hrData,40,120,'#ff6384');";
    html += "drawChart(spo2Ctx,spo2Data,90,100,'#36a2eb');";
    html += "drawMultiChart(orientCtx,[{data:yawData,color:'#ff6384'},{data:pitchData,color:'#36a2eb'},{data:rollData,color:'#4bc0c0'}],-180,180);";
    html += "drawChart(accelXCtx,accelXData,-20,20,'#ff6384');";
    html += "drawChart(accelYCtx,accelYData,-20,20,'#36a2eb');";
    html += "drawChart(accelZCtx,accelZData,-20,20,'#4bc0c0');";
    html += "drawChart(pwmCtx,pwmData,0,100,'#ffce56');}";
    html += "window.addEventListener('resize',resizeCanvas);";
    html += "resizeCanvas();";
    html += "function updateData(){";
    html += "fetch('/api/sensor/heartrate').then(r=>r.json()).then(data=>{";
    html += "document.getElementById('hr').textContent=data.hr_valid&&data.heart_rate>0?data.heart_rate+' bpm':'--';";
    html += "document.getElementById('hr').className='value '+(data.hr_valid?'valid':'invalid');";
    html += "document.getElementById('spo2').textContent=data.spo2_valid&&data.spo2>0?data.spo2+' %':'--';";
    html += "document.getElementById('spo2').className='value '+(data.spo2_valid?'valid':'invalid');";
    html += "document.getElementById('temp').textContent=data.temperature.toFixed(1)+' °C';";
    html += "document.getElementById('ir').textContent=data.ir_value;";
    html += "if(data.hr_valid&&data.heart_rate>0&&data.heart_rate>=40&&data.heart_rate<=120){";
    html += "hrData.push(data.heart_rate);if(hrData.length>maxPoints)hrData.shift();";
    html += "drawChart(hrCtx,hrData,40,120,'#ff6384');}";
    html += "if(data.spo2_valid&&data.spo2>0&&data.spo2>=90&&data.spo2<=100){";
    html += "spo2Data.push(data.spo2);if(spo2Data.length>maxPoints)spo2Data.shift();";
    html += "drawChart(spo2Ctx,spo2Data,90,100,'#36a2eb');}";
    html += "}).catch(e=>console.error('Error:',e));";
    html += "fetch('/api/sensor/accel').then(r=>r.json()).then(data=>{";
    html += "if(data.valid){";
    html += "document.getElementById('yaw').textContent=data.Yaw.toFixed(1)+' °';";
    html += "document.getElementById('pitch').textContent=data.Pitch.toFixed(1)+' °';";
    html += "document.getElementById('roll').textContent=data.Roll.toFixed(1)+' °';";
    html += "yawData.push(data.Yaw);if(yawData.length>maxPoints)yawData.shift();";
    html += "pitchData.push(data.Pitch);if(pitchData.length>maxPoints)pitchData.shift();";
    html += "rollData.push(data.Roll);if(rollData.length>maxPoints)rollData.shift();";
    html += "accelXData.push(data.x_accel);if(accelXData.length>maxPoints)accelXData.shift();";
    html += "accelYData.push(data.y_accel);if(accelYData.length>maxPoints)accelYData.shift();";
    html += "accelZData.push(data.z_accel);if(accelZData.length>maxPoints)accelZData.shift();";
    html += "drawMultiChart(orientCtx,[{data:yawData,color:'#ff6384'},{data:pitchData,color:'#36a2eb'},{data:rollData,color:'#4bc0c0'}],-180,180);";
    html += "drawChart(accelXCtx,accelXData,-20,20,'#ff6384');";
    html += "drawChart(accelYCtx,accelYData,-20,20,'#36a2eb');";
    html += "drawChart(accelZCtx,accelZData,-20,20,'#4bc0c0');}";
    html += "}).catch(e=>console.error('Error:',e));";
    html += "fetch('/api/pwm/get').then(r=>r.json()).then(data=>{";
    html += "document.getElementById('pwm').textContent=data.value.toFixed(1)+' %';";
    html += "pwmData.push(data.value);if(pwmData.length>maxPoints)pwmData.shift();";
    html += "drawChart(pwmCtx,pwmData,0,100,'#ffce56');";
    html += "}).catch(e=>console.error('Error:',e));}";
    html += "updateData();setInterval(updateData,1000);";
    html += "</script>";
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
 * @brief PWM取得API "/api/pwm/get"
 */
void handlePWMGetAPI() {
    float value = getPWM();
    String json = "{\"value\":" + String(value) + "}";
    server.send(200, "application/json", json);
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
    server.on("/api/pwm/get", HTTP_GET, handlePWMGetAPI);
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