// FTP Client Lib
#include "ESP32_FTPClient.h"
#include "/home/alex/ownCloud/alex/Arduino/libraries/cyberconfig/cy_ftp_cfg.h" 
 
ESP32_FTPClient ftp(ftp_srv_3, ftp_usr_3, ftp_pass_3);

void FTP_upload( uint8_t * buf, size_t len)
{
    //URL and picture name
String pic_name = "";
//String pic_url = "";

  // Rename the picture with the time string
  char dateTimeString[40];
  DBG( year());
  sprintf(dateTimeString, "%04d%02d%02d_%02d%02d%02d", year(), month(), day(), hour(), minute(), second());

  pic_name = String(dateTimeString) + ".jpg";
  DBG("Camera capture success, saved as:");
  DBG(pic_name);
  
  DBG("Uploading via FTP");
  ftp.OpenConnection();

  //Create a file and write the image data to it;
  ftp.InitFile("Type I");
  ftp.ChangeWorkDir("/ESP32Cam/"); // change it to reflect your directory
  const char *f_name = pic_name.c_str();
  ftp.NewFile(f_name);
  //ftp.WriteData(fb->buf, fb->len);
  ftp.WriteData(buf, len);
    ftp.CloseFile();

  // Breath, withouth delay URL failed to update.
  delay(200);
}
