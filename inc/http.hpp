/*
HTTP通信を行うヘッダファイル
ESP32-C3用 Wi-Fiアクセスポイント & WebAPIサーバー
*/

#ifndef HTTP_HPP
#define HTTP_HPP

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// 前方宣言
class MAX30105;
class Adafruit_BNO08x_RVC;

// Wi-Fi設定
#define AP_SSID "ESP32-C3_AP"      // アクセスポイント名
#define AP_PASSWORD "dcon2026_kazuma"     // パスワード（8文字以上）
#define AP_CHANNEL 1               // チャンネル
#define AP_HIDDEN false            // SSIDを隠すか
#define AP_MAX_CONNECTIONS 4       // 最大接続数

// センサー初期化関数（setupHTTPの前に呼び出す）
void initHTTPSensors(MAX30105& max30105, Adafruit_BNO08x_RVC& bno08x);

// Webサーバー初期化

void setupHTTP();

// Webサーバーのループ処理（loop()内で呼ぶ）
void handleHTTP();

// Wi-Fi接続数を取得
int getConnectedClients();

#endif // HTTP_HPP