
#include "Arduino.h"

#include "WiFi.h"
#include <WiFiClient.h>
#include <WebServer.h>
#include <FtpServer.h>

#include "app_http_server.h"

#include "FS.h"
#include "FFat.h"
//#include "SPIFFS.h"

#include "hwpins.h"
#include "traces.h"

FtpServer ftp_server;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial

const char * ftp_username = "user";
const char * ftp_password = "pass";


IPAddress local_IP(192, 168, 0, 71);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 0, 1); //optional
IPAddress secondaryDNS(192, 168, 0, 1); //optional

#include "wifi_secret.h"  // copy the wifi_secret.h.template to wifi_secret.h with your AP data


RTC_DATA_ATTR int bootCount = 0;

TGpioPin  pin_led(2, false);
unsigned g_hbcounter = 0;
unsigned last_hb_time = 0;

void setup()
{
  traces_init();

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (ESP_SLEEP_WAKEUP_TIMER == wakeup_reason)
  {
    TRACE("wakeup happened.\n");
  }
  else
  {
    TRACE("\n\n--------------------------------\n");
  }

  pin_led.Setup(PINCFG_OUTPUT);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    TRACE("WiFi config error!\n");
  }
  else
  {
    TRACE("ESP Mac Address: %s\n", WiFi.macAddress().c_str());

    TRACE("Initializing WiFi with IP=%s ...\n", local_IP.toString().c_str());

    unsigned st = micros();

    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED)
    {
      if (micros() - st > 5000000)
      {
        TRACE("WiFi connect timeout!\n");
        break;
      }
      delay(1);
      //TRACE(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      TRACE("WiFi connected in %u us.\n", micros() - st);
    }
  }

  if (FFat.begin(true))  // formatOnFail = true
  {
    TRACE("FFat initialized.\n");

    TRACE("Initializing FTP server with username=\"%s\", password=\"%s\" ...\n", ftp_username, ftp_password);
    ftp_server.begin(ftp_username, ftp_password);    //username, password for ftp.   (default 21, 50009 for PASV)
  }
  else
  {
    TRACE("Error initializing SPIFFS!\n");
  }

  app_http_server_setup();

  last_hb_time = 0;

  TRACE("Setup done.\n");
}

//-----------------------------------------------------

int wifi_scan_state = 0;
int wifi_scan_result = 0;
unsigned wifi_scan_cnt = 0;

void task_wifi_scan(void * aparam)
{
  wifi_scan_result = WiFi.scanNetworks();  // this takes several seconds !
  wifi_scan_state = 2; // signalize scan finished.

  vTaskDelete(nullptr);
}

void show_wifi_networks()
{
  TRACE("WiFi Scan Result:\n");
  if (0 == wifi_scan_result)
  {
    TRACE("  No networks found.\n");
  }
  else
  {
    TRACE("  %i networks found:\n", wifi_scan_result);
    for (int i = 0; i < wifi_scan_result; ++i)
    {
      TRACE("  %2i: %s (%i)", i + 1, WiFiScanClass(WiFi).SSID(i).c_str(), WiFiScanClass(WiFi).RSSI(i));
      if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN)
      {
        TRACE(", encrypted\n");
      }
      else
      {
        TRACE(", open\n");
      }
    }
  }
}

unsigned last_scan_time = 0;

void run_wifi_scan()
{
  unsigned t = micros();
  if (0 == wifi_scan_state)
  {
    if (t - last_scan_time > 7000)  // start scanning every 7s
    {
      wifi_scan_state = 1;
      TRACE("Starting wifi scan task...\n");
      xTaskCreatePinnedToCore(
          task_wifi_scan,
          "wifi_scan",
          4192,  // stack size
          nullptr,
          5, // priority,
          nullptr,  // pvCreatedTask,
          0 // xCoreID
      );
      ++wifi_scan_cnt;
      last_scan_time = t;
    }
  }
  else if (1 == wifi_scan_state)
  {
    // wait until completes
  }
  else if (2 == wifi_scan_state)
  {
    // wifi scan completed.
    TRACE("%u. WiFi scan completed in %u us\n", wifi_scan_cnt, t - last_scan_time);
    show_wifi_networks();
    wifi_scan_state = 0;

#if 0
    TRACE("Sleeping for 5 seconds...\n");
    esp_sleep_enable_timer_wakeup(5 * 1000000);
    esp_deep_sleep_start();
#endif
  }
}

void _callback(FtpOperation ftpOperation, unsigned int freeSpace, unsigned int totalSpace)
{
  switch (ftpOperation) {
    case FTP_CONNECT:
      Serial.println(F("FTP: Connected!"));
      break;
    case FTP_DISCONNECT:
      Serial.println(F("FTP: Disconnected!"));
      break;
    case FTP_FREE_SPACE_CHANGE:
      Serial.printf("FTP: Free space change, free %u of %u!\n", freeSpace, totalSpace);
      break;
    default:
      break;
  }
};
void _transferCallback(FtpTransferOperation ftpOperation, const char* name, unsigned int transferredSize){
  switch (ftpOperation) {
    case FTP_UPLOAD_START:
      Serial.println(F("FTP: Upload start!"));
      break;
    case FTP_UPLOAD:
      Serial.printf("FTP: Upload of file %s byte %u\n", name, transferredSize);
      break;
    case FTP_TRANSFER_STOP:
      Serial.println(F("FTP: Finish transfer!"));
      break;
    case FTP_TRANSFER_ERROR:
      Serial.println(F("FTP: Transfer error!"));
      break;
    default:
      break;
  }

  /* FTP_UPLOAD_START = 0,
   * FTP_UPLOAD = 1,
   *
   * FTP_DOWNLOAD_START = 2,
   * FTP_DOWNLOAD = 3,
   *
   * FTP_TRANSFER_STOP = 4,
   * FTP_DOWNLOAD_STOP = 4,
   * FTP_UPLOAD_STOP = 4,
   *
   * FTP_TRANSFER_ERROR = 5,
   * FTP_DOWNLOAD_ERROR = 5,
   * FTP_UPLOAD_ERROR = 5
   */
};

void loop()
{
  unsigned t = micros();

  if (t - last_hb_time > 1000*1000)
  {
    ++g_hbcounter;
    pin_led.SetTo(g_hbcounter & 1);
    last_hb_time = t;
  }

  //ftp_server.setCallback(_callback);
  //ftp_server.setTransferCallback(_transferCallback);

  ftp_server.handleFTP();

  app_http_server_run();

  //run_wifi_scan();
}
