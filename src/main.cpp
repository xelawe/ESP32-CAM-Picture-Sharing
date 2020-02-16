/******************************************************************************

ESP32-CAM remote image access via FTP. Take pictures with ESP32 and upload it via FTP making it accessible for the outisde network. 
Leonardo Bispo
July - 2019
https://github.com/ldab/ESP32-CAM-Picture-Sharing

Distributed as-is; no warranty is given.

******************************************************************************/
#include "Arduino.h"

// Enable Debug interface and serial prints over UART1
#define DEGUB_ESP
#ifdef DEGUB_ESP
#define DBG(x) Serial.println(x)
#else
#define DBG(...)
#endif

// Wifi related Libs
#include <WiFi.h>
#include <WiFiClient.h>
#include "/home/alex/ownCloud/alex/Arduino/libraries/cyberconfig/cy_wifi_cfg.h"

#include "time_tool.h"
#include "camera_tool.h"
//#include "esp_timer.h"
#include "ftp_tool.h"

// Variable marked with this attribute will keep its value during a deep sleep / wake cycle.
RTC_DATA_ATTR uint64_t bootCount = 0;

//void deep_sleep(void);
//void FTP_upload(void);
//void FTP_upload( uint8_t * buf, size_t len);

void setup()
{
#ifdef DEGUB_ESP
  Serial.begin(115200);
  Serial.setDebugOutput(true);
#endif

  init_cam();

  // Enable timer wakeup for ESP32 sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);

  WiFi.begin(wifi_ssid_i, wifi_pass_i);
  DBG("\nConnecting to WiFi");

  while (WiFi.status() != WL_CONNECTED && millis() < CON_TIMEOUT)
  {
    delay(500);
    Serial.print(".");
  }

  if (!WiFi.isConnected())
  {
    DBG("Failed to connect to WiFi, going to sleep");
    deep_sleep();
  }

  DBG("");
  DBG("WiFi connected");
  DBG(WiFi.localIP());

  configTime(0, 0, ntpServer);
}

void loop()
{

  printLocalTime();

  // Take picture
  if (take_picture())
  {
    FTP_upload(fb->buf, fb->len);

    deep_sleep();
  }

  else
  {
    DBG("Capture failed, sleeping");
    deep_sleep();
  }

  if (millis() > CON_TIMEOUT)
  {
    DBG("Timeout");

    deep_sleep();
  }
}