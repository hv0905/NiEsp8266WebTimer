#include "hitokotoHelper.h"

const char* FAILED_TXT = "连接失败.";

String getHitokoto() {
    Serial.println("Connecting hitokoto...");
    WiFiClientSecure wifiClient;
    wifiClient.setInsecure(); // Use it since we are just getting some public data.
    HTTPClient client;
    if (!client.begin(wifiClient, "https://v1.hitokoto.cn/?encode=text")) {
        Serial.println("Connect fail.");
        return String(FAILED_TXT);
    }
    Serial.println("Connected. now send HTTP GET request");
    auto code = client.GET();
    Serial.print("Connection code is");
    Serial.println(code);
    if (code != 200) {
        return String(FAILED_TXT);
    }
    Serial.println("Receiving payload");
    auto payload = client.getString();
    Serial.println(payload);
    client.end();

    return payload;
}