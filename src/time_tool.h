#include <TimeLib.h>
#include "time.h"
 
// Connection timeout;
#define CON_TIMEOUT 10 * 1000 // milliseconds

// Not using Deep Sleep on PCB because TPL5110 timer takes over.
#define TIME_TO_SLEEP (uint64_t)10 * 60 * 1000 * 1000 // microseconds

const char *ntpServer = "pool.ntp.org";

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo), 10000) // Versuche 10 s zu Synchronisieren 
  {
    Serial.println("Failed to obtain time");
    return;
  }
  setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, (timeinfo.tm_mon + 1), (timeinfo.tm_year + 1900));
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void deep_sleep()
{
  DBG("Going to sleep after: " + String(millis()) + "ms");
  Serial.flush();

  esp_deep_sleep_start();
}