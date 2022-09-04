/*
 * http_server.cpp
 *
 *  Created on: Sep 4, 2022
 *      Author: vitya
 */


#include "Arduino.h"
#include <WebServer.h>
#include "FFat.h"

WebServer app_http(80);

void app_http_not_found()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += app_http.uri();
  message += "\nMethod: ";
  message += (app_http.method() == HTTP_GET ? "GET" : "POST");
  message += "\nArguments: ";
  message += app_http.args();
  message += "\n";
  for (uint8_t i=0; i<app_http.args(); i++)
  {
    message += " " + app_http.argName(i) + ": " + app_http.arg(i) + "\n";
  }
  app_http.send(404, "text/plain", message);
}

void app_http_server_setup()
{
  app_http.onNotFound(app_http_not_found);

  app_http.serveStatic("/", FFat, "/index.html");
  app_http.serveStatic("/index.html", FFat, "/index.html");
  app_http.serveStatic("/default.css", FFat, "/default.css");
  app_http.serveStatic("/dc.js", FFat, "/dc.js");
  app_http.serveStatic("/jstools2.js", FFat, "/jstools2.js");

  app_http.begin();
}

void app_http_server_run()
{
  app_http.handleClient();
}
