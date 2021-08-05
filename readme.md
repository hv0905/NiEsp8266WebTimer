# ESP8266 Network Clock

## 功能

 1. 自动联网对时
 2. 倒计日
 3. 获取并显示一言内容

## 安装

### 显示屏

本代码适用于ESP8266 NodeMCU + 12864显示屏

7pin SPI引脚，正面看，从左到右依次为GND、VCC、D0、D1、RES、DC、CS

```
ESP8266 ---  OLED
3V    ---  VCC
G     ---  GND
D7    ---  D1
D5    ---  D0
D2orD8---  CS
D1    ---  DC
RST   ---  RES
```

4pin IIC引脚，正面看，从左到右依次为GND、VCC、SCL、SDA

```
ESP8266  ---  OLED
3.3V     ---  VCC
G (GND)  ---  GND
D1(GPIO5)---  SCL
D2(GPIO4)---  SDA
```

本构建默认使用IIC接口。

### 按钮引脚

BTN1(pull up): D6 用于显示一言

BTN2(pull up): D7 用于显示配置页面/重置配置

### 刷入

使用ESP8266 Download Tool刷入固件即可.

### 日志

串口监视器： 波特率:115200 编码:UTF-8

## 配置

1. 首次上电后， 用任意设备连接热点WiFi：EspTimer，等待登录页弹出或浏览器输入`192.168.4.1` 即可进入配置页面.
2. Wifi配置保存后， 可以通过连接同一wifi并访问设备ip方式进入配置页面. 设备ip可经由上级网关查看或短接D7-GND引脚查看.
3. 若系统上电10秒内连接wifi失败， 用于配置的Wifi热点将重新开启， 届时设备屏幕也会指示此状态，此时可重新连接热点wifi完成配置。

## 版权
Copyright 2021 EdgeNeko  
基于GPLv3许可开源  
本固件在极大程度上基于[flyAkari](https://github.com/flyAkari)开发的[ESP8266_Network_Clock](https://github.com/flyAkari/ESP8266_Network_Clock)开发。  
在此提出特别感谢。  
