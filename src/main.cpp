#include <Arduino.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <EEPROM.h>
#include <U8g2lib.h>
#include <inttypes.h>
#include "ntpHelper.h"
#include "hitokotoHelper.h"
#include "webpage.h"

#define uint unsigned int
#define APP_FONT u8g2_font_wqy12_t_gb2312a
#define TIME_FONT u8g2_font_logisoso24_tr

//若屏幕使用SH1106，只需把SSD1306改为SH1106即可
//U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/4, /* dc=*/5, /* reset=*/3);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 4, /* data=*/ 5); //D-duino

const char *AP_NAME = "NiEspTimer"; //自定义8266AP热点名
const byte DNS_PORT = 53;       //DNS端口号默认为53
IPAddress apIP(192, 168, 4, 1); //8266 APIP
DNSServer dnsServer;
ESP8266WebServer server(80);

void oledClockDisplay();
void initdisplay();
void HitokotoDisplay();
void MaintainceDisplay();
void resetConfig();
void initSyncer();

time_t prevDisplay = 0; //当时钟已经显示

const int DEFAULT_VERSION = 1;
typedef struct
{
    int version;
    int timezone;
    char wifi_ssid[32];
    char wifi_pw[64];
    bool enable_countdown;
    time_t countdown_deadline;
    char countdown_description[32];
    char hitokoto_url[128];

} config_type;
config_type config;

void saveConfig()
{ //存储配置到"EEPROM"
    Serial.println("save config");
    EEPROM.begin(sizeof(config));
    uint8_t *p = (uint8_t *)(&config);
    for (uint i = 0; i < sizeof(config); i++)
    {
        EEPROM.write(i, *(p + i));
    }
    EEPROM.commit(); //此操作会消耗flash写入次数
}

int daysBetweenTwoTimestamp(time_t srcStamp, time_t dstStamp)
{ //倒数日返回正数，正数日返回负数，同一天为0
    if (dstStamp >= srcStamp)
    {
        return (dstStamp + 86399 - srcStamp) / 86400;
    }
    else
    {
        return (srcStamp - dstStamp) / 86400 * (-1);
    }
}

void loadConfig()
{ //从"EEPROM"加载配置
    Serial.println("load config");
    EEPROM.begin(sizeof(config));
    uint8_t *p = (uint8_t *)(&config);
    for (uint i = 0; i < sizeof(config); i++)
    {
        *(p + i) = EEPROM.read(i);
    }
    if (config.version != DEFAULT_VERSION)
    {
        // reset config
        resetConfig();
    }
}

void printConfig()
{
    Serial.println("===CONFIG===");
    Serial.print("TimeZone: ");
    Serial.printf("%d\n", config.timezone);
    Serial.print("WIFI SSID: ");
    Serial.println(config.wifi_ssid);
    Serial.print("WIFI Password: ");
    Serial.println(config.wifi_pw);
    Serial.println("===END CONFIG===");
}

void connectWiFi();

void handleRoot()
{
    String result = String(page_html);
    result.replace("${wifi_ssid}", config.wifi_ssid);
    result.replace("${wifi_pw}", config.wifi_pw);
    result.replace("${timezone}", String(config.timezone).c_str());
    result.replace("${enable_countdown}", config.enable_countdown ? "checked" : "");
    char tmpBuffer[12] = {0};
    sprintf(tmpBuffer, "%04d-%02d-%02d", year(config.countdown_deadline), month(config.countdown_deadline), day(config.countdown_deadline));
    result.replace("${countdown_deadline}", tmpBuffer);
    result.replace("${countdown_description}", config.countdown_description);
    result.replace("${hitokoto_url}", config.hitokoto_url);
    server.send(200, "text/html", result);
}
void handleRootPost()
{
    Serial.println("handleRootPost");
    if (server.hasArg("wifi_ssid"))
    {
        Serial.print("ssid:");
        strncpy(config.wifi_ssid, server.arg("wifi_ssid").c_str(), sizeof(config.wifi_ssid));
        Serial.println(config.wifi_ssid);
    }
    else
    {
        Serial.println("[WebServer]Error, SSID not found!");
        server.send(200, "text/html", "<meta charset='UTF-8'>Error, SSID not found!"); //返回错误页面
        return;
    }
    if (server.hasArg("wifi_pw"))
    {
        Serial.print("password:");
        strncpy(config.wifi_pw, server.arg("wifi_pw").c_str(), sizeof(config.wifi_pw));
        Serial.println(config.wifi_pw);
    }
    else
    {
        Serial.println("[WebServer]Error, PASSWORD not found!");
        server.send(200, "text/html", "<meta charset='UTF-8'>Error, PASSWORD not found!");
        return;
    }
    if (server.hasArg("timezone"))
    {
        Serial.print("timezone:");
        char timeZone_s[4];
        strcpy(timeZone_s, server.arg("timezone").c_str());
        int timeZone = atoi(timeZone_s);
        if (timeZone > 13 || timeZone < -13)
        {
            timeZone = 8;
        }
        Serial.println(timeZone);
        config.timezone = timeZone;
    }
    else
    {
        Serial.println("[WebServer]Error, TIMEZONE not found!");
        server.send(200, "text/html", "<meta charset='UTF-8'>Error, TIMEZONE not found!");
        return;
    }
    if (server.hasArg("enable_countdown") && server.arg("enable_countdown").compareTo("on") == 0)
    {
        // countdown enabled
        config.enable_countdown = true;
        if (server.hasArg("countdown_deadline"))
        {
            tmElements_t date;
            int year, month, day;
            sscanf(server.arg("countdown_deadline").c_str(), "%04d-%02d-%02d", &year, &month, &day);
            date.Year = (uint8_t)(year - 1970);
            date.Month = (uint8_t)month;
            date.Day = (uint8_t)day;
            date.Hour = date.Minute = date.Second = 0;
            config.countdown_deadline = makeTime(date);
            Serial.print("countdown_dateline:");
            Serial.println(config.countdown_deadline);
        }
        else
        {
            Serial.println("[WebServer]Error, countdown_deadline not found!");
            server.send(200, "text/html", "<meta charset='UTF-8'>Error, countdown_deadline not found!");
            return;
        }
        if (server.hasArg("countdown_description"))
        {
            strncpy(config.countdown_description, server.arg("countdown_description").c_str(), sizeof(config.countdown_description));
        }
        else
        {
            memset(config.countdown_description, 0, sizeof(config.countdown_description));
        }
    }
    else
    {
        config.enable_countdown = false;
    }
    if (server.hasArg("hitokoto_url")) {
        strncpy(config.hitokoto_url, server.arg("hitokoto_url").c_str(), sizeof(config.hitokoto_url));
    }
    server.send(200, "text/html", "<meta charset='UTF-8'>设定完成"); //返回保存成功页面
    delay(2000);
    saveConfig();
    //一切设定完成，连接wifi
    
    if (WiFi.status() != WL_CONNECTED) {
        ESP.restart();
    }
}

void startServer()
{
    server.on("/", HTTP_GET, handleRoot);      //设置主页回调函数
    server.onNotFound(handleRoot);             //设置无法响应的http请求的回调函数
    server.on("/saveConfig", HTTP_POST, handleRootPost); //设置Post请求回调函数
    server.begin();                            //启动WebServer
    Serial.println("WebServer started!");
}

void connectWiFi()
{
    WiFi.mode(WIFI_STA);       //切换为STA模式
    WiFi.setAutoConnect(true); //设置自动连接
    WiFi.begin(config.wifi_ssid, config.wifi_pw);
    Serial.print("Connect WiFi");
    int count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        count++;
        if (count > 20)
        { //10秒过去依然没有自动连上，开启Web配网功能，可视情况调整等待时长
            Serial.println("Timeout! AutoConnect failed");
            WiFi.mode(WIFI_AP); //开热点
            WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
            if (WiFi.softAP(AP_NAME))
            {
                Serial.println("ESP8266 SoftAP is on");
            }
            if (dnsServer.start(DNS_PORT, "*", apIP))
            { //判断将所有地址映射到esp8266的ip上是否成功
                Serial.println("start dnsserver success.");
            }
            else
                Serial.println("start dnsserver failed.");
            Serial.println("AP Mode started. Please connect to Default AP and change the settings.");
            break; //启动WebServer后便跳出while循环，回到loop
        }
        Serial.print(".");
        if (WiFi.status() == WL_CONNECT_FAILED)
        {
            Serial.print("password:");
            Serial.print(WiFi.psk().c_str());
            Serial.println(" is incorrect");
        }
        if (WiFi.status() == WL_NO_SSID_AVAIL)
        {
            Serial.print("configured SSID:");
            Serial.print(WiFi.SSID().c_str());
            Serial.println(" cannot be reached");
        }
    }
    Serial.println("");
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi Connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        server.stop();
        dnsServer.stop();
    }
}

void setup()
{
    pinMode(D6, INPUT_PULLUP);
    pinMode(D7, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    while (!Serial)
        continue;
    initdisplay();
    u8g2.clearBuffer();
    u8g2.setFont(APP_FONT);
    u8g2.setCursor(30, 30);
    u8g2.print("Ni Esp Timer");
    u8g2.setCursor(28, 46);
    u8g2.print("正在连接网络");
    u8g2.sendBuffer();
    Serial.println("OLED Ready");
    Serial.print("Loading configure from EEPROM...");
    loadConfig();
    printConfig();
    Serial.print("Connecting WiFi...");
    WiFi.hostname("NiEspTimer");
    connectWiFi();
    Serial.print("Starting web server...");
    startServer();
    if (WiFi.status() == WL_CONNECTED)
    {
        initSyncer();
    }
    else
    {
        u8g2.clearBuffer();
        u8g2.setFont(APP_FONT);
        u8g2.setCursor(0, 14);
        u8g2.print("WL网络连接失败");
        u8g2.setCursor(0, 30);
        u8g2.print("连接热点完成配置");
        u8g2.setCursor(0, 46);
        u8g2.print(AP_NAME);
        u8g2.setCursor(0, 62);
        u8g2.print(apIP.toString());
        u8g2.sendBuffer();
    }
    digitalWrite(LED_BUILTIN, HIGH); // Initialzation done, pwroff the led.
}

void loop()
{
    dnsServer.processNextRequest();
    server.handleClient();

    // D6
    if (digitalRead(D6) == LOW)
    {
        // Show Hitokoto
        HitokotoDisplay();
        delay(8000); // 显示8s
        return;
    }

    // D7
    if (digitalRead(D7) == LOW)
    {
        MaintainceDisplay();
        auto prevTime = millis();
        bool goReset = true;
        delay(500); // 消除抖动
        while (true)
        {
            if (digitalRead(D7) != LOW)
            {
                goReset = false;
            }
            if (millis() - prevTime >= 7500)
            {
                if (goReset)
                {
                    resetConfig();
                    saveConfig();
                    u8g2.clearBuffer();
                    u8g2.setFont(APP_FONT);
                    u8g2.setCursor(44, 38);
                    u8g2.print("已重置");
                    u8g2.sendBuffer();
                    delay(2000);
                    ESP.restart();
                }
                break;
            }
            delay(10);
        }

        return;
    }

    if (timeStatus() != timeNotSet)
    {
        if (now() != prevDisplay)
        { //时间改变时更新显示
            prevDisplay = now();
            oledClockDisplay();
        }
    }
}

void initdisplay()
{
    u8g2.begin();
    u8g2.enableUTF8Print();
}

void oledClockDisplay()
{
    int years, months, days, hours, minutes, seconds, weekdays;
    years = year();
    months = month();
    days = day();
    hours = hour();
    minutes = minute();
    seconds = second();
    weekdays = weekday();
    Serial.printf("%d/%d/%d %d:%d:%d Weekday:%d\n", years, months, days, hours, minutes, seconds, weekdays);
    u8g2.clearBuffer();
    u8g2.setFont(APP_FONT);
    u8g2.setCursor(0, 14);
    if (get_isNtpConnected())
    {
        if (config.enable_countdown)
        {
            if (strlen(config.countdown_description) == 0)
            {
                u8g2.print("距目标日");
            }
            else
            {
                u8g2.print(config.countdown_description);
            }

            u8g2.printf(":%d天", daysBetweenTwoTimestamp(now(), config.countdown_deadline));
        }
        else
        {
            if (hours >= 12)
            {
                u8g2.print("下午   ");
            }
            else
            {
                u8g2.print("上午   ");
            }
            if (config.timezone >= 0)
            {
                u8g2.print("(UTC+");
                u8g2.print(config.timezone);
                u8g2.print(")");
            }
            else
            {
                u8g2.print("(UTC");
                u8g2.print(config.timezone);
                u8g2.print(")");
            }
        }
    }
    else
        u8g2.print("检查网络连接"); //如果上次对时失败，则会显示无网络
    String currentTime = "";
    if (hours < 10)
        currentTime += 0;
    currentTime += hours;
    currentTime += ":";
    if (minutes < 10)
        currentTime += 0;
    currentTime += minutes;
    currentTime += ":";
    if (seconds < 10)
        currentTime += 0;
    currentTime += seconds;
    String currentDay = "";
    currentDay += years;
    currentDay += "/";
    if (months < 10)
        currentDay += 0;
    currentDay += months;
    currentDay += "/";
    if (days < 10)
        currentDay += 0;
    currentDay += days;

    u8g2.setFont(TIME_FONT);
    u8g2.setCursor(0, 44);
    u8g2.print(currentTime);
    u8g2.setCursor(0, 61);
    u8g2.setFont(APP_FONT);
    u8g2.print(currentDay);
    u8g2.print("星期");
    if (weekdays == 1)
        u8g2.print("日");
    else if (weekdays == 2)
        u8g2.print("一");
    else if (weekdays == 3)
        u8g2.print("二");
    else if (weekdays == 4)
        u8g2.print("三");
    else if (weekdays == 5)
        u8g2.print("四");
    else if (weekdays == 6)
        u8g2.print("五");
    else if (weekdays == 7)
        u8g2.print("六");
    u8g2.sendBuffer();
}

void HitokotoDisplay()
{

    Serial.write("Attempt to display Hitokoto...");
    // set load screen
    u8g2.clearBuffer();
    u8g2.setFont(APP_FONT);
    u8g2.setCursor(44, 38);
    u8g2.print("加载中");
    u8g2.sendBuffer();

    auto hitokoto = getHitokoto(config.hitokoto_url);
    hitokoto.replace("。", ".");
    hitokoto.replace("，", ",");
    int len = hitokoto.length();
    int currY = 14;
    int charPos = 0;
    u8g2.clearBuffer();
    for (int i = 0; i < 4; i++)
    {
        u8g2.setCursor(0, currY);
        u8g2.print(hitokoto.substring(charPos, charPos + 30));
        charPos += 30;
        if (charPos > len)
        {
            break;
        }
        currY += 16;
    }
    u8g2.sendBuffer();
}

void MaintainceDisplay()
{
    u8g2.clearBuffer();
    u8g2.setFont(APP_FONT);
    u8g2.setCursor(0, 14);
    u8g2.print("管理页面");
    u8g2.setCursor(0, 30);
    u8g2.print(WiFi.localIP().toString());
    u8g2.setCursor(0, 46);
    u8g2.print(config.wifi_ssid);
    u8g2.setCursor(0, 62);
    u8g2.print("按住8s可重置");
    u8g2.sendBuffer();
}

void resetConfig()
{
    config = {};
    config.timezone = 8;
    config.version = DEFAULT_VERSION;
}

void initSyncer()
{
    initNtp();
    setSyncProvider([]() -> time_t
                    { return getNtpTime(config.timezone); });
    setSyncInterval(300); //每300秒同步一次时间
}