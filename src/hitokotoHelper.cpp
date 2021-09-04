#include "hitokotoHelper.h"

const char* FAILED_TXT = "连接失败.";
const char* DEFAULT_URL = "https://v1.hitokoto.cn/?encode=text";

String getHitokoto(String url) {
    String *conUrl;
    if (!url.isEmpty()) {
        conUrl = &url;
    } else {
        String s = DEFAULT_URL;
        conUrl = &s;
    }
    Serial.println("Connecting hitokoto...");
    WiFiClient *wclient;
    if (url.startsWith("https")) {
        WiFiClientSecure wifiClient;
        wifiClient.setInsecure(); // Use it since we are just getting some public data.
        // WARN: 由于Arduino限制原因，此处获取不会对https证书进行合规性检验，请谨慎用于隐私数据的拉取，或修改代码以验证网站SSL证书指纹
        wclient = &wifiClient;
    } else {
        WiFiClient wificlient;
        wclient = &wificlient;
    }
    
    HTTPClient client;

    if (!client.begin(*wclient, *conUrl)) {
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
