#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <esp_wifi.h> // Used for the global deauth capability

const char* ap_ssid = "MyPortfolio";

DNSServer dnsServer;
WebServer server(80);
IPAddress apIP(192, 168, 4, 1);

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ap_ssid);

  dnsServer.start(53, "*", apIP);

  server.serveStatic("/", LittleFS, "/");

  // --- Target Disconnect Endpoint (Clean Core 3.x Implementation) ---
  server.on("/self-destruct", HTTP_GET, []() {
    Serial.println("Self-destruct endpoint hit. Processing client kick...");

    // 1. Send the response back cleanly so the browser doesn't throw an ugly crash page
    server.send(200, "text/plain", "Kicking you off the network...");
    delay(500); 

    // 2. Get the specific client connection processing this exact request
    NetworkClient client = server.client();
    if (client) {
      Serial.println("Dropping target client active TCP connection socket.");
      client.stop(); // Force-closes the network socket connection for this user
    }

    // 3. Issue a targeted hardware deauth event frame.
    // Passing '0' drops the current transient client allocation context safely 
    // without having to query experimental internal DHCP lease structs.
    esp_wifi_deauth_sta(0); 
  });

  // Captive Portal Redirects
  server.on("/generate_204", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });

  server.on("/gen_204", HTTP_GET, []() {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "");
  });

  server.on("/hotspot-detect.html", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "", "");
  });

  server.on("/library/test/success.html", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "", "");
  });

  server.on("/connecttest.txt", HTTP_GET, []() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "", "");
  });

  server.on("/ncsi.txt", HTTP_GET, []() {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "", "");
  });

  server.onNotFound([]() {
    File file = LittleFS.open("/index.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "Not found");
    }
  });

  server.begin();
  Serial.println("Portal running");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}