#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include "WifiManager.hpp"


//==============================
//    HARDWARE SETTINGS
//==============================
#define BAUD 115200
#define HTTP_PORT 80

#define STASSID "SSID"
#define STAPSK  "CorrectHorseBatteryStaple"

//==============================
//    NETWORK SETTINGS
//==============================
const char* ssid = STASSID;
const char* password = STAPSK;


//==============================
//       GLOBALS
//==============================
WifiManager wifi_manager;
ESP8266WebServer httpServer(HTTP_PORT);
String host = "awesome-iot-device";

//==============================
//    SETUP FUNCTIONS
//==============================
void setup() {
	Serial.begin(BAUD);

	wifi_manager.SetHost(host);
	wifi_manager.Setup(httpServer);
	delay(1000);

	SetupWiFi();
	SetupWebpages();
}

void SetupWebpages() {
	//Add custom pages
	//httpServer.on("/page1", HTTP_POST, handlePage1);
	//httpServer.on("/page2", HTTP_GET, handlePage2);

	httpServer.begin();
}

void SetupWiFi(void) {
	Serial.println("Booting...");
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	
	int wifi_attempts = 0;

	while (WiFi.waitForConnectResult() != WL_CONNECTED) {
		WiFi.begin(ssid, password);
		Serial.println("WiFi failed, retrying.");
		delay(100);
		wifi_attempts += 1;
		if (wifi_attempts > 10){
			return;
		}
	}

	Serial.printf("Open http://%s.local in your browser\n", host.c_str());
}

void loop() {
	wifi_manager.CheckStatus();
	wifi_manager.dnsServer.processNextRequest();
	httpServer.handleClient();
}
