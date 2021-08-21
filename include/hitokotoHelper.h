#pragma once

#ifndef __HITOKOTO_HELPER__
#define __HITOKOTO_HELPER__

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

String getHitokoto(String url);

#endif