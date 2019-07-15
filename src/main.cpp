/******************************************************************************

ESP32-CAM remote image access via FTP. Take pictures with ESP32 and upload it via FTP making it accessible for the outisde network. 
Leonardo Bispo
July - 2019
https://github.com/ldab/ESP32-CAM-Picture-Sharing

Distributed as-is; no warranty is given.

******************************************************************************/
#include "Arduino.h"

/* Comment this out to disable prints and save space */
//#define BLYNK_DEBUG
//#define BLYNK_PRINT Serial
#define BLYNK_NO_BUILTIN
#define BLYNK_NO_FLOAT

// Enable Debug interface and serial prints over UART1
#define DEGUB_ESP

// Blynk related Libs
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"

#include "esp_http_server.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
//#include "soc/soc.h"           //disable brownout problems
//#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include "dl_lib.h"

//#include "Octocat_asdata.h"

// Connection timeout;
#define CON_TIMEOUT   10*1000                     // milliseconds

// Not using Deep Sleep on PCB because TPL5110 timer takes over.
#define TIME_TO_SLEEP (uint64_t)50*60*1000*1000   // microseconds

// Assing pin names
#define _SDA          22
#define _SCL          21
#define RGB_R         33
#define RGB_G         25
#define RGB_B         26
#define DONE          32
#define BATT          36                        //ADC1 CHANNEL 0 *ADC2 chanel can't be used in conjunction with Wi-Fi https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/adc.html

#ifdef DEGUB_ESP
  #define DBG(x) Serial.println(x)
#else 
  #define DBG(...)
#endif

// FTP Client Lib
#include "ESP32_FTPClient.h"

// Go to the Project Settings (nut icon) and get Auth Token in the Blynk App.
char auth[] = "";

// Your WiFi credentials.
char ssid[] = "";
char pass[] = "";

// FTP Server credentials
char ftp_server[] = "files.000webhost.com";
char ftp_user[]   = "";
char ftp_pass[]   = "";

// Camera buffer, URL and picture name
camera_fb_t *fb = NULL;
String pic_name = "";
String pic_url  = "";

// Variable marked with this attribute will keep its value during a deep sleep / wake cycle.
RTC_DATA_ATTR uint64_t bootCount = 0;
RTC_DATA_ATTR uint64_t time_unix = 0;

BlynkTimer timer;
WidgetRTC rtc;
ESP32_FTPClient ftp (ftp_server, ftp_user, ftp_pass);

void FTP_upload( void );
bool take_picture(void);

void setup()
{
#ifdef DEGUB_ESP
  Serial.begin(115200);
  Serial.setDebugOutput(true);
#endif

  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  //init with high specs to pre-allocate larger buffers
  if(psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } 
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.print("Camera init failed with error 0x%x");
    DBG(err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
 
  //drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

  // Enable timer wakeup for ESP32 sleep
  esp_sleep_enable_timer_wakeup( TIME_TO_SLEEP );

  WiFi.begin( ssid, pass );

  while ( WiFi.status() != WL_CONNECTED && millis() < CON_TIMEOUT )
  {
    delay(500);
    Serial.print(".");
  }

  if( !WiFi.isConnected() )
  {
    DBG("Failed to connect to WiFi, going to sleep");
    //esp_deep_sleep_start();
  }

  DBG("");
  DBG("WiFi connected");
  DBG( WiFi.localIP() );

  Blynk.config( auth );
  rtc.begin();

}

void loop()
{
  Blynk.run();

  // Take picture
  if( take_picture() )
  {
    while( !Blynk.connected() ) delay(1);

    FTP_upload();

    esp_deep_sleep_start();
  }

  // After sending the picture sleep.
  //esp_deep_sleep_start();
}

bool take_picture()
{
  DBG("Taking picture now");

  // Take picture
  
  fb = esp_camera_fb_get();  
  if(!fb)
  {
    DBG("Camera capture failed");
    return false;
  }
  
  // Rename the picture with the time string
  pic_name = String( now() ) + ".jpg";

  return true;
}

void FTP_upload()
{
  DBG("Uploading via FTP");
  ftp.OpenConnection();
  
  //Create a file and write the image data to it;
  ftp.InitFile("Type I");
  ftp.ChangeWorkDir("/public_html/zyro/gallery_gen/");
  const char *f_name = pic_name.c_str();
  ftp.NewFile( f_name );
  ftp.WriteData(fb->buf, fb->len);
  ftp.CloseFile();

  // Change URL on Blynk App
  pic_url += pic_name;
  DBG("Change App URL to: ");
  DBG( pic_url );
  Blynk.setProperty(V0, "url", 1, pic_url);

  // Breath, withouth delay URL failed to update.
  delay(500);

}
