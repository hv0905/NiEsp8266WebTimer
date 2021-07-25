#pragma once

#ifndef __NTP_HELPER__
#define __NTP_HELPER__
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

const int NTP_PACKET_SIZE = 48;     // NTP时间在消息的前48个字节里
static const char NTP_SERVER_NAME[] = "ntp1.aliyun.com"; //NTP服务器，阿里云
const unsigned int LOCAL_PORT = 8888; // 用于侦听UDP数据包的本地端口

void initNtp();
time_t getNtpTime(int timezone);
void sendNTPpacket(IPAddress &address);
bool get_isNtpConnected();

#endif