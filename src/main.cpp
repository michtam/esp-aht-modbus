/*
  ModbusRTU ESP8266/ESP32
  Read multiple coils from slave device example

  (c)2019 Alexander Emelianov (a.m.emelianov@gmail.com)
  https://github.com/emelianov/modbus-esp8266

  modified 13 May 2020
  by brainelectronics

  This code is licensed under the BSD New License. See LICENSE.txt for more info.
*/
#include <iterator>
#include <iostream>

// Modbus lib https://github.com/emelianov/modbus-esp8266
#include <ModbusRTU.h>
#include <ModbusIP_ESP8266.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <SoftwareSerial.h>
#include <ArduinoOTA.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "pages.h"
#include "registers.h"
#include "httpServer.h"
#include "wifi_config.h"

ModbusRTU mb_rtu;
ModbusIP mb_tcp;
AsyncWebServer webserver(80);
SoftwareSerial ModbusSerial(4, 5);

const char* ssid = SSID;
const char* password = PSK;
const char* hostname = HOSTNAME;

uint16_t RegInitVal = 0;

int test_var = 69;

bool shouldReboot = false;

uint16_t cReg = 0;

char BUILD_REVISION[] = AUTO_VERSION;
float all_reg_values[std::end(modbus_registers) - std::begin(modbus_registers)];

String getUptimeString() {
  uint16_t days;
  uint8_t hours;
  uint8_t minutes;
  uint8_t seconds;

  #define SECS_PER_MIN  60
  #define SECS_PER_HOUR 3600
  #define SECS_PER_DAY  86400

  time_t uptime = millis() / 1000;

  seconds = uptime % SECS_PER_MIN;
  uptime -= seconds;
  minutes = (uptime % SECS_PER_HOUR) / SECS_PER_MIN;
  uptime -= minutes * SECS_PER_MIN;
  hours = (uptime % SECS_PER_DAY) / SECS_PER_HOUR;
  uptime -= hours * SECS_PER_HOUR;
  days = uptime / SECS_PER_DAY;

  char buffer[20];
  sprintf(buffer, "%4u days %02d:%02d:%02d", days, hours, minutes, seconds);
  return buffer;
}

String processor(const String& var)
{
  if(var == "MODBUS_REGISTER_DATA"){
    String data_table = "";

    data_table += "<tr>";
    data_table += "<td>Uptime</td>";
    data_table += "<td>" + getUptimeString() + "</td>";
    data_table += "<td>d h:m:s</td>";
    data_table += "</tr>";

    auto array_length = std::end(modbus_registers) - std::begin(modbus_registers);
    for ( int i = 0; i < array_length; i++) {
      data_table += "<tr>";
      data_table += "<td>" + String(i) +"</td>";
      data_table += "<td>" + String(all_reg_values[i]) + "</td>";
      data_table += "<td>XYZ</td>";
      data_table += "</tr>";
    };


    return data_table;
  }
  if(var == "BUILD_REVISION"){

    return BUILD_REVISION;
  }
  return String();
}


void config_webserver(){

  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_page, processor);
  });

  webserver.onNotFound(notFound);

  webserver.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });

  webserver.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  webserver.onNotFound(onRequest);
  webserver.onFileUpload(onUpload);
  webserver.onRequestBody(onBody);

}

void setup() {

  // Init serials
  Serial.begin(115200);
  ModbusSerial.begin(9600, SWSERIAL_8N1);

  // Init wifi
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(SSID, PSK);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  // Init OTA and webui
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.begin();

  // Init modbus rtu and tcp
  mb_rtu.begin(&ModbusSerial);
  mb_rtu.master();
  mb_tcp.server(502);

  // Get size of array with register's addresses
  auto array_length = std::end(modbus_registers) - std::begin(modbus_registers);

  // Init local input register
  for ( int i = 0; i < array_length; i++) {
    mb_tcp.addIreg(modbus_registers[i], RegInitVal);
    mb_tcp.addIreg(modbus_registers[i] + 0x0001, RegInitVal);
  };

  // Init webserver
  config_webserver();
  webserver.begin();

}


void loop() {
  ArduinoOTA.handle();
  MDNS.update();

  if(shouldReboot){
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

  auto array_length = std::end(modbus_registers) - std::begin(modbus_registers);
  if (!mb_rtu.slave()) {

    switch (array_length - cReg) {
      case 0:
        cReg = 0;
        break;

      default:
//        Serial.printf_P("(%d) Pull register: 0x%04X (%d) \n", cReg, modbus_registers[cReg], modbus_registers[cReg]);
        mb_rtu.pullIreg(1, modbus_registers[cReg], modbus_registers[cReg], 1);
        uint16_t var_reg = mb_tcp.Ireg(modbus_registers[cReg]);
        float var = *((float*)&var_reg);
//        Serial.printf_P("(%d) Pull register: 0x%04X (%d) with value raw: 0x%04X%04X / %d | decoded: %.2f   \n", cReg, modbus_registers[cReg], modbus_registers[cReg],var_reg1, var_reg2, var_reg, var);
        cReg++;
    }

  };

  mb_rtu.task();
  mb_tcp.task();

}
