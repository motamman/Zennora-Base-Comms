#ifndef ZENNORA_BASECOMMS_H
#define ZENNORA_BASECOMMS_H

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <AsyncFsWebServer.h>
#include "ZennoraFileSystemAndConfig.h"
#include <LittleFS.h>

struct Config;  // Forward declaration to avoid circular dependency

// Define ZennoraBaseComms class to handle Wi-Fi, OTA, and Webserver
class ZennoraBaseComms {
public:
    ZennoraBaseComms(Config& config, class ZennoraFileSystemAndConfig* fsConfig);
    bool setupWiFi();
    bool setupOTA();
    bool setupMDNS();
    void startWebServer();
    void startWebServerX();
    void handleOTA();
    void handleWebServer();
    String getLocalIP() const;  // Method to get the local IP address
    int getRSSI() const;        // Method to get RSSI
    void handleRoot();
    void handleSave();
    void startAP();
    
    
private:
    Config& config;  // Reference to shared configuration
    WebServer server;  // Web server object
    class ZennoraFileSystemAndConfig* fsConfig; // Pointer to ZennoraFileSystemAndConfig

};

#endif
