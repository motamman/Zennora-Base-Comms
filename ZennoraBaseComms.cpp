#include "ZennoraBaseComms.h"
#include "ZennoraFileSystemAndConfig.h"  // Include only in the .cpp to avoid circular reference

// Constructor definition, initializing with reference to config and pointer to ZennoraFileSystemAndConfig
ZennoraBaseComms::ZennoraBaseComms(Config& configRef, ZennoraFileSystemAndConfig* fsConfigPtr)
    : config(configRef), server(80), fsConfig(fsConfigPtr) {}

// Setup WiFi connection
bool ZennoraBaseComms::setupWiFi() {
    Serial.print("Connecting to Wi-Fi: ");
    Serial.println(config.ssid);

    WiFi.begin(config.ssid, config.password);
    int attempts = 0;

    // Attempt to connect to Wi-Fi
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());

        // Validate device name
        if (config.deviceName == nullptr || strlen(config.deviceName) == 0) {
            Serial.println("Error: deviceName is invalid.");
            return false; // Invalid device name
        }

        Serial.println("Wi-Fi setup complete.");
        return true; // Successfully connected to Wi-Fi
    } else {
        Serial.println("\nFailed to connect to Wi-Fi. Starting AP mode...");
        startAP(); // Start access point for provisioning
        return false; // Wi-Fi connection failed
    }
}

bool ZennoraBaseComms::setupMDNS() {
    // Ensure device name is valid
    if (config.deviceName == nullptr || strlen(config.deviceName) == 0) {
        Serial.println("Error: deviceName is invalid. mDNS setup failed.");
        return false; // Invalid device name
    }

    // Start mDNS
    if (!MDNS.begin(config.deviceName)) {
        Serial.println("Error: Failed to start mDNS responder.");
        return false; // mDNS initialization failed
    }

    Serial.println("mDNS responder started successfully.");
    return true;
}


// Start the access point for Wi-Fi provisioning
void ZennoraBaseComms::startAP() {
    WiFi.softAP("Zennora_Device_Setup");
    Serial.println("AP mode started. Connect to 'Zennora Device Setup' to configure Wi-Fi.");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [this]() {
        String html = "<html><body><h1>Wi-Fi Configuration</h1><form action=\"/save\" method=\"POST\">";
        html += "SSID: <input type=\"text\" name=\"ssid\"><br>";
        html += "Password: <input type=\"password\" name=\"password\"><br>";
        html += "<input type=\"submit\" value=\"Save\"></form></body></html>";
        server.send(200, "text/html", html);
    });

    server.on("/save", HTTP_POST, [this]() {
        String newSSID = server.arg("ssid");
        String newPassword = server.arg("password");

        if (newSSID.length() > 0 && newPassword.length() > 0) {
            strcpy(config.ssid, newSSID.c_str());
            strcpy(config.password, newPassword.c_str());

            // Save the new configuration using fsConfig
            if (fsConfig) {
                fsConfig->saveConfig("/config/configSensor.json");
            }

            server.send(200, "text/html", "<html><body><h1>Credentials Saved! Rebooting...</h1></body></html>");
            delay(1000);
            ESP.restart();  // Restart to apply the new settings
        } else {
            server.send(400, "text/html", "<html><body><h1>Invalid Input</h1></body></html>");
        }
    });

    server.begin();
}


// Setup Over-The-Air (OTA) update
bool ZennoraBaseComms::setupOTA() {
    if (config.deviceName == nullptr || strlen(config.deviceName) == 0) {
        Serial.println("Error: Invalid device name. OTA setup failed.");
        return false; // Return false if deviceName is invalid
    }

    ArduinoOTA.setHostname(config.deviceName);

    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        Serial.println("Start updating " + type);
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("OTA Initialized.");
    return true; // Return true if initialization is successful
}

// Start the web server
void ZennoraBaseComms::startWebServer() {
    server.on("/", [this]() { handleRoot(); });
    server.on("/save", HTTP_POST, [this]() { handleSave(); });
    server.begin();
    Serial.println("Web server started.");
}



// Handle the root webpage (for configuration)
void ZennoraBaseComms::handleRoot() {

      String html = "<html><body><h1>Configure Network, Signal K, and Circuits</h1><form action=\"/save\" method=\"POST\">";

      // Network settings
      html += "SSID: <input type=\"text\" name=\"ssid\" value=\"" + String(config.ssid) + "\"><br>";
      html += "Password: <input type=\"text\" name=\"password\" value=\"" + String(config.password) + "\"><br>";
      html += "Context Name: <input type=\"text\" name=\"contextName\" value=\"" + String(config.contextName) + "\"><br>";
      html += "MMSI: <input type=\"text\" name=\"mmsi\" value=\"" + String(config.mmsi) + "\"><br>";
      
      html += "Signal K Server: <input type=\"text\" name=\"signalKServer\" value=\"" + String(config.signalKServer) + "\"><br>";
      html += "Signal K Port: <input type=\"number\" name=\"signalKPort\" value=\"" + String(config.signalKPort) + "\"><br>";
      html += "Receive UDP Server: <input type=\"text\" name=\"receiveUDPServer\" value=\"" + String(config.receiveUDPServer) + "\"><br>";
      html += "Receive UDP Port: <input type=\"number\" name=\"receiveUDPport\" value=\"" + String(config.receiveUDPport) + "\"><br>";
      html += "MQTT Server: <input type=\"text\" name=\"mqttServer\" value=\"" + String(config.mqttServer) + "\"><br>";
      html += "MQTT Port: <input type=\"number\" name=\"mqttPort\" value=\"" + String(config.mqttPort) + "\"><br>";
      html += "MQTT User: <input type=\"text\" name=\"mqttUser\" value=\"" + String(config.mqttUser) + "\"><br>";
      html += "MQTT Password: <input type=\"text\" name=\"mqttPassword\" value=\"" + String(config.mqttPassword) + "\"><br>";

      html += "TCP Port: <input type=\"number\" name=\"tcpPort\" value=\"" + String(config.tcpPort) + "\"><br>";
      html += "Device Name: <input type=\"text\" name=\"deviceName\" value=\"" + String(config.deviceName) + "\"><br>";
      html += "Display Name: <input type=\"text\" name=\"displayName\" value=\"" + String(config.displayName) + "\"><br>";
      html += "Time Server: <input type=\"text\" name=\"timeServer\" value=\"" + String(config.timeServer) + "\"><br>";
      html += "GMT Offset (sec): <input type=\"number\" name=\"gmtOffset_sec\" value=\"" + String(config.gmtOffset_sec) + "\"><br>";
    html += "Daylight Savings Time Offset (sec): <input type=\"number\" name=\"daylightOffset_sec\" value=\"" + String(config.daylightOffset_sec) + "\"><br>";
    html += "Primary loop speed (ms): <input type=\"number\" name=\"loopSpeed\" value=\"" + String(config.loopSpeed) + "\"><br>";

      // Device features checkboxes
      html += "<h2>Device Features</h2>";
      html += "<label>Has Compass: <input type=\"checkbox\" name=\"hasCompass\" " + String(config.hasCompass ? "checked" : "") + "></label><br>";
      html += "<label>Has Multiplexer: <input type=\"checkbox\" name=\"hasMultiplexer\" " + String(config.hasMultiplexer ? "checked" : "") + "></label><br>";
      html += "<label>Has GPS: <input type=\"checkbox\" name=\"hasGPS\" " + String(config.hasGPS ? "checked" : "") + "></label><br>";

      // Conditional compass settings
      if (config.hasCompass) {
        html += "<h3>Compass Settings</h3>";
        html += "Compass Offset: <input type=\"number\" name=\"compassOffset\" value=\"" + String(config.compassOffset) + "\"><br>";
        html += "X axis offset: <input type=\"number\" name=\"magXOffset\" value=\"" + String(config.magXOffset) + "\"><br>";
        html += "Y axis offset: <input type=\"number\" name=\"magYOffset\" value=\"" + String(config.magYOffset) + "\"><br>";
        html += "Z axis offset: <input type=\"number\" name=\"magZOffset\" value=\"" + String(config.magZOffset) + "\"><br>";
      }

      // Display existing circuits only if multiplexer is present
      if (config.hasMultiplexer) {
        html += "<h2>Circuits</h2>";
        for (int i = 0; i < config.circuits.size(); i++) {
          Circuit &circuit = config.circuits[i];
          html += "<fieldset><legend>Circuit " + String(i + 1) + "</legend>";
          html += "<input type=\"hidden\" name=\"circuitId" + String(i) + "\" value=\"" + String(circuit.id) + "\">";
          html += "Name: <input type=\"text\" name=\"circuitName" + String(circuit.id) + "\" value=\"" + circuit.name + "\"><br>";
          html += "Identifier: <input type=\"text\" name=\"circuitIdentifier" + String(circuit.id) + "\" value=\"" + circuit.identifier + "\"><br>";
          html += "Multiplexer: <input type=\"number\" name=\"circuitMultiplexer" + String(circuit.id) + "\" value=\"" + String(circuit.multiplexer) + "\"><br>";
          html += "Channel: <input type=\"number\" name=\"circuitChannel" + String(circuit.id) + "\" value=\"" + String(circuit.channel) + "\"><br>";
          html += "I2C Address: <input type=\"text\" name=\"circuitAddress" + String(circuit.id) + "\" value=\"" + String(circuit.address, HEX) + "\"><br>";
          html += "<label>Remove Circuit: <input type=\"checkbox\" name=\"removeCircuit" + String(circuit.id) + "\"></label>";
          html += "</fieldset><br>";
        }

        // Add option for a new circuit
        html += "<h3>Add New Circuit</h3>";
        html += "Name: <input type=\"text\" name=\"newCircuitName\"><br>";
        html += "Identifier: <input type=\"text\" name=\"newCircuitIdentifier\"><br>";
        html += "Multiplexer: <input type=\"number\" name=\"newCircuitMultiplexer\"><br>";
        html += "Channel: <input type=\"number\" name=\"newCircuitChannel\"><br>";
        html += "I2C Address (hex): <input type=\"text\" name=\"newCircuitAddress\"><br>";
      }

      html += "<input type=\"submit\" value=\"Save\"></form></body></html>";
      server.send(200, "text/html", html);
}

void ZennoraBaseComms::handleSave() {

      // Save network settings
      if (server.hasArg("ssid")) strcpy(config.ssid, server.arg("ssid").c_str());
      if (server.hasArg("password")) strcpy(config.password, server.arg("password").c_str());
      if (server.hasArg("contextName")) strcpy(config.contextName, server.arg("contextName").c_str());
      if (server.hasArg("mmsi")) strcpy(config.mmsi, server.arg("mmsi").c_str());
      
      if (server.hasArg("signalKServer")) strcpy(config.signalKServer, server.arg("signalKServer").c_str());
      if (server.hasArg("signalKPort")) config.signalKPort = server.arg("signalKPort").toInt();

      if (server.hasArg("receiveUDPServer")) strncpy(config.receiveUDPServer, server.arg("receiveUDPServer").c_str(), sizeof(config.receiveUDPServer));
      if (server.hasArg("receiveUDPport")) config.receiveUDPport = server.arg("receiveUDPport").toInt();
      if (server.hasArg("mqttServer")) strncpy(config.mqttServer, server.arg("mqttServer").c_str(), sizeof(config.mqttServer));
      if (server.hasArg("mqttPort")) config.mqttPort = server.arg("mqttPort").toInt();
      if (server.hasArg("tcpPort")) config.tcpPort = server.arg("tcpPort").toInt();

      if (server.hasArg("mqttUser")) strncpy(config.mqttUser, server.arg("mqttUser").c_str(),sizeof(config.mqttUser));
      if (server.hasArg("mqttPassword")) strncpy(config.mqttPassword, server.arg("mqttPassword").c_str(),sizeof(config.mqttPassword));


      
      if (server.hasArg("deviceName")) strcpy(config.deviceName, server.arg("deviceName").c_str());
      if (server.hasArg("displayName")) strcpy(config.displayName, server.arg("displayName").c_str());
      if (server.hasArg("timeServer")) strcpy(config.timeServer, server.arg("timeServer").c_str());
      if (server.hasArg("gmtOffset_sec")) config.gmtOffset_sec = server.arg("gmtOffset_sec").toInt();
      if (server.hasArg("daylightOffset_sec")) config.daylightOffset_sec = server.arg("daylightOffset_sec").toInt();
    if (server.hasArg("loopSpeed")) config.loopSpeed = server.arg("loopSpeed").toInt();
  
    
      if (server.hasArg("hasCompass")) config.hasCompass = true; else config.hasCompass = false;
      if (server.hasArg("hasMultiplexer")) config.hasMultiplexer = true; else config.hasMultiplexer = false;
      if (server.hasArg("hasGPS")) config.hasGPS = true; else config.hasGPS = false;
      if (server.hasArg("compassOffset")) config.compassOffset = server.arg("compassOffset").toInt();
      if (server.hasArg("magXOffset")) config.magXOffset = server.arg("magXOffset").toInt();
      if (server.hasArg("magYOffset")) config.magYOffset = server.arg("magYOffset").toInt();
      if (server.hasArg("magZOffset")) config.magZOffset = server.arg("magZOffset").toInt();

      // Create a new vector for circuits to keep
      std::vector<Circuit> updatedCircuits;
      for (int i = 0; i < config.circuits.size(); i++) {
        Circuit &circuit = config.circuits[i];
        if (!(server.hasArg("removeCircuit" + String(circuit.id)) && server.arg("removeCircuit" + String(circuit.id)) == "on")) {
          // Keep the circuit if it is not marked for removal
          updatedCircuits.push_back(circuit);
        }
      }

      // Replace the old circuits vector with the updated one
      config.circuits = updatedCircuits;

      // Update circuit details
      for (int i = 0; i < config.circuits.size(); i++) {
        Circuit &circuit = config.circuits[i];
        if (server.hasArg("circuitName" + String(circuit.id))) circuit.name = server.arg("circuitName" + String(circuit.id));
        if (server.hasArg("circuitIdentifier" + String(circuit.id))) circuit.identifier = server.arg("circuitIdentifier" + String(circuit.id));
        if (server.hasArg("circuitMultiplexer" + String(circuit.id))) circuit.multiplexer = server.arg("circuitMultiplexer" + String(circuit.id)).toInt();
        if (server.hasArg("circuitChannel" + String(circuit.id))) circuit.channel = server.arg("circuitChannel" + String(circuit.id)).toInt();
        if (server.hasArg("circuitAddress" + String(circuit.id))) circuit.address = strtol(server.arg("circuitAddress" + String(circuit.id)).c_str(), NULL, 16);
      }

      // Add new circuit if specified (ensure required fields are filled)
      if (server.hasArg("newCircuitName") && !server.arg("newCircuitName").isEmpty() &&
          server.hasArg("newCircuitMultiplexer") && !server.arg("newCircuitMultiplexer").isEmpty() &&
          server.hasArg("newCircuitChannel") && !server.arg("newCircuitChannel").isEmpty() &&
          server.hasArg("newCircuitAddress") && !server.arg("newCircuitAddress").isEmpty()) {
        Circuit newCircuit;
        newCircuit.id = millis(); // Use a unique ID based on time
        newCircuit.name = server.arg("newCircuitName");
        newCircuit.identifier = server.arg("newCircuitIdentifier");
        newCircuit.multiplexer = server.arg("newCircuitMultiplexer").toInt();
        newCircuit.channel = server.arg("newCircuitChannel").toInt();
        newCircuit.address = strtol(server.arg("newCircuitAddress").c_str(), NULL, 16);
        config.circuits.push_back(newCircuit);
      }

      // Ensure the updated configuration is saved
      if (fsConfig->saveConfig("/config/configSensor.json")) {
        String html = "<html><body><h1>Configuration Saved</h1><p>The device will reboot shortly to apply new settings.</p>";
        html += "<p>After reboot, wait 20 seconds to be redirected automatically. Or click on the button</p>";
        html += "<button onclick=\"window.location.href='/'\">Go Back to Configuration</button>";
        html += "<script>setTimeout(function(){ window.location.href = '/'; }, 20000);</script>";
        html += "</body></html>";
        
        server.send(200, "text/html", html);

        delay(2000);  // Short delay for the user to see the confirmation
        ESP.restart(); // Restart to apply new settings
      } else {
        server.send(500, "text/html", "<html><body><h1>Failed to Save Configuration</h1></body></html>");
      }
}

    
// Handle OTA updates and the web server in the main loop
void ZennoraBaseComms::handleOTA() {
    ArduinoOTA.handle();
}

void ZennoraBaseComms::handleWebServer() {
    server.handleClient();
}


String ZennoraBaseComms::getLocalIP() const {
    return WiFi.localIP().toString();
}

int ZennoraBaseComms::getRSSI() const {
    return WiFi.RSSI();
}


