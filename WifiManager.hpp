#include "arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>


class WifiManager {

private:
	ESP8266WebServer* server;
    const byte DNS_PORT = 53;

    String host;
    String softAP_ssid;
    String softAP_password;

	unsigned long lastConnectTry;
	unsigned int status;

	/* Soft AP network parameters */
	IPAddress apIP;
	IPAddress netMsk;

    void (*on_connect_fn)(void);
    void (*on_disconnect_fn)(void);

public:

    DNSServer dnsServer;

	char ssid[33];
	char password[65];

	bool loaded;
    bool reconnect;
    bool InAPMode;
    bool wifi_details_updated;

    bool isConnected;

    String MAC;

	WifiManager() {
        SetHost("ESP");
        softAP_password = "12345678";
        InAPMode = false;
	}

    WifiManager(String host) {
        SetHost(host);
        softAP_password = "12345678";
        InAPMode = false;
    }

    WifiManager(String host, String default_password) {
        SetHost(host);
        softAP_password = default_password;
        InAPMode = false;
    }

    void SetHost(String host) {
        this->host = host;
        this->softAP_ssid = host;
        GetMAC();
    }

    void GetMAC() {
        /*
        * Update the Host and SSID with mac address
        */
	    byte mac[6];
	    WiFi.macAddress(mac);
	    char mac_[20];
	    sprintf(mac_, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        char mac_short[20];
        sprintf(mac_short, "%02X%02X%02X", mac[3], mac[4], mac[5]);

	    MAC = String(mac_);
        host += "_" + MAC;

        softAP_ssid += "_" + String(mac_short);

        //myHostname = host.c_str();
    }

    void SetWebServer(ESP8266WebServer& server) {
        this->server = &server;
        this->server->on("/", std::bind(&WifiManager::handleRoot, this));
        this->server->on("/wifi", std::bind(&WifiManager::handleWifi, this));
        this->server->on("/wifisave", std::bind(&WifiManager::handleWifiSave, this));
        this->server->on("/generate_204", std::bind(&WifiManager::handleRoot, this));  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
        this->server->on("/fwlink", std::bind(&WifiManager::handleRoot, this));  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
        this->server->onNotFound(std::bind(&WifiManager::handleNotFound, this));
    }

	void Setup(ESP8266WebServer& server) {
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", apIP);

        loaded = false;
        apIP = IPAddress(172, 16, 0, 1);
        netMsk = IPAddress(255, 255, 255, 0);

        status = WL_IDLE_STATUS;
        lastConnectTry = 0;
        loadCredentials();

        this->server = &server;
        this->server->on("/", std::bind(&WifiManager::handleRoot, this));
		this->server->on("/wifi", std::bind(&WifiManager::handleWifi, this));
		this->server->on("/wifisave", std::bind(&WifiManager::handleWifiSave, this));
		this->server->on("/generate_204", std::bind(&WifiManager::handleRoot, this));  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
		this->server->on("/fwlink", std::bind(&WifiManager::handleRoot, this));  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
		this->server->onNotFound(std::bind(&WifiManager::handleNotFound, this));
	}

    void handleTest() {
        Serial.println("handleTest");
        Serial.println(host.c_str());
        Serial.println(ssid);
    }

	void EnableSoftAP() {
        isConnected = false;
		WiFi.softAPConfig(apIP, apIP, netMsk);
		WiFi.softAP(softAP_ssid, softAP_password);
        InAPMode = true;
	}

	int Connect() {
		if (loaded) {
            Serial.println("Connecting as wifi client...");
			WiFi.disconnect();
			WiFi.begin(ssid, password);
			int connRes = WiFi.waitForConnectResult();
            Serial.print("connRes: ");
            Serial.println(ResultToString(connRes));
            lastConnectTry = millis();

            if (connRes == WL_CONNECTED) {
                //success
                reconnect = false;
                InAPMode = false;
                isConnected = true;
                call_on_connect();
            }
            else {
                reconnect = true;
                isConnected = false;
            }

			return connRes;
		}
	}

    String ResultToString(int res) {
        switch (res)
        {
        case 0:
            return "Idle";
        case 1:
            return "No SSID";
        case 2:
            return "Scan Complete";
        case 3:
            return "Connected";
        case 4:
            return "Connection Failed";
        case 5:
            return "Connection Lost";
        case 6:
            return "Disconnected";
        default:
            break;
        }
    }

	void DisableAP() {
		WiFi.mode(WIFI_STA);
        InAPMode = false;
	}

	void loadCredentials() {
		EEPROM.begin(512);
		EEPROM.get(0, ssid);
		EEPROM.get(0 + sizeof(ssid), password);
		char ok[2 + 1];
		EEPROM.get(0 + sizeof(ssid) + sizeof(password), ok);
		EEPROM.end();
		if (String(ok) != String("OK")) {
			ssid[0] = 0;
			password[0] = 0;
		}
		Serial.println("Recovered credentials:");
		Serial.println(ssid);
		Serial.println(strlen(password) > 0 ? "********" : "<no password>");
        loaded = true;
        reconnect = true;
	}

	void saveCredentials() {
		EEPROM.begin(512);
		EEPROM.put(0, ssid);
		EEPROM.put(0 + sizeof(ssid), password);
		char ok[2 + 1] = "OK";
		EEPROM.put(0 + sizeof(ssid) + sizeof(password), ok);
		EEPROM.commit();
		EEPROM.end();
	}


    /** Handle root or redirect to captive portal */
    void handleRoot() {
        if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
            return;
        }
        this->server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        this->server->sendHeader("Pragma", "no-cache");
        this->server->sendHeader("Expires", "-1");

        String Page;
        Page += F(
            "<!DOCTYPE html><html lang='en'><head>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>CaptivePortal</title></head><body>"
            "<h1>HELLO WORLD!!</h1>");
        if (this->server->client().localIP() == apIP) {
            Page += String(F("<p>You are connected through the soft AP: ")) + softAP_ssid + F("</p>");
        }
        else {
            Page += String(F("<p>You are connected through the wifi network: ")) + ssid + F("</p>");
        }
        Page += F(
            "<p>You may want to <a href='/wifi'>config the wifi connection</a>.</p>"
            "</body></html>");

        this->server->send(200, "text/html", Page);
    }

    /** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
    boolean captivePortal() {
        if (!isIp(this->server->hostHeader()) && this->server->hostHeader() != (host + ".local")) {
            Serial.println("Request redirected to captive portal");
            this->server->sendHeader("Location", String("http://") + toStringIp(this->server->client().localIP()), true);
            this->server->send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
            this->server->client().stop(); // Stop is needed because we sent no content length
            return true;
        }
        return false;
    }

    /** Wifi config page handler */
    void handleWifi() {
        this->server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        this->server->sendHeader("Pragma", "no-cache");
        this->server->sendHeader("Expires", "-1");

        String Page;
        Page += F(
            "<!DOCTYPE html><html lang='en'><head>"
            "<meta name='viewport' content='width=device-width'>"
            "<title>CaptivePortal</title></head><body>"
            "<h1>Wifi config</h1>");
        if (this->server->client().localIP() == apIP) {
            Page += String(F("<p>You are connected through the soft AP: ")) + softAP_ssid + F("</p>");
        }
        else {
            Page += String(F("<p>You are connected through the wifi network: ")) + ssid + F("</p>");
        }
        Page +=
            String(F(
                "\r\n<br />"
                "<table><tr><th align='left'>SoftAP config</th></tr>"
                "<tr><td>SSID ")) +
            softAP_ssid +
            F("</td></tr>"
                "<tr><td>IP ") +
            toStringIp(WiFi.softAPIP()) +
            F("</td></tr>"
                "</table>"
                "\r\n<br />"
                "<table><tr><th align='left'>WLAN config</th></tr>"
                "<tr><td>SSID ") +
            String(ssid) +
            F("</td></tr>"
                "<tr><td>IP ") +
            toStringIp(WiFi.localIP()) +
            F("</td></tr>"
                "</table>"
                "\r\n<br />"
                "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>");
        Serial.println("scan start");
        int n = WiFi.scanNetworks();
        Serial.println("scan done");
        if (n > 0) {
            for (int i = 0; i < n; i++) {
                Page += String(F("\r\n<tr><td>SSID ")) + WiFi.SSID(i) + ((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? F(" ") : F(" *")) + F(" (") + WiFi.RSSI(i) + F(")</td></tr>");
            }
        }
        else {
            Page += F("<tr><td>No WLAN found</td></tr>");
        }
        Page += F(
            "</table>"
            "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
            "<input type='text' placeholder='network' name='n'/>"
            "<br /><input type='password' placeholder='password' name='p'/>"
            "<br /><input type='submit' value='Connect/Disconnect'/></form>"
            "<p>You may want to <a href='/'>return to the home page</a>.</p>"
            "</body></html>");
        this->server->send(200, "text/html", Page);
        this->server->client().stop(); // Stop is needed because we sent no content length
    }

    /** Handle the WLAN save form and redirect to WLAN config page again */
    void handleWifiSave() {
        Serial.println("wifi save");
        this->server->arg("n").toCharArray(ssid, sizeof(ssid) - 1);
        this->server->arg("p").toCharArray(password, sizeof(password) - 1);
        this->server->sendHeader("Location", "wifi", true);
        this->server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        this->server->sendHeader("Pragma", "no-cache");
        this->server->sendHeader("Expires", "-1");
        this->server->send(302, "text/plain", "");    // Empty content inhibits Content-length header so we have to close the socket ourselves.
        this->server->client().stop(); // Stop is needed because we sent no content length
        //saveCredentials();
        saveCredentials();
        reconnect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
        Serial.print("reconnect? ");
        Serial.println(reconnect);
        if (reconnect) {
            Connect();
        }
    }

    void handleNotFound() {
        if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
            return;
        }
        String message = F("File Not Found\n\n");
        message += F("URI: ");
        message += this->server->uri();
        message += F("\nMethod: ");
        message += (this->server->method() == HTTP_GET) ? "GET" : "POST";
        message += F("\nArguments: ");
        message += this->server->args();
        message += F("\n");

        for (uint8_t i = 0; i < this->server->args(); i++) {
            message += String(F(" ")) + this->server->argName(i) + F(": ") + this->server->arg(i) + F("\n");
        }
        this->server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        this->server->sendHeader("Pragma", "no-cache");
        this->server->sendHeader("Expires", "-1");
        this->server->send(404, "text/plain", message);
    }

    //**Is this an IP ? */
    boolean isIp(String str) {
        for (size_t i = 0; i < str.length(); i++) {
            int c = str.charAt(i);
            if (c != '.' && (c < '0' || c > '9')) {
                return false;
            }
        }
        return true;
    }

    /** IP to String? */
    String toStringIp(IPAddress ip) {
        String res = "";
        for (int i = 0; i < 3; i++) {
            res += String((ip >> (8 * i)) & 0xFF) + ".";
        }
        res += String(((ip >> 8 * 3)) & 0xFF);
        return res;
    }

    void CheckStatus() {
        unsigned int s = WiFi.status();
        if (s == 0 && millis() > (lastConnectTry + 60000) && !InAPMode) {
            /* If WLAN disconnected and idle try to connect */
            /* Don't set retry time too low as retry interfere the softAP operation */
            reconnect = true;
            Serial.println("Connect requested");
            Connect();
        }
        if (status != s) { // WLAN status change
            Serial.print("Wifi status changed: ");
            Serial.println(ResultToString(s));
            status = s;
            if (s == WL_CONNECTED) {
                DisableAP();
                /* Just connected to WLAN */
                Serial.println("");
                Serial.print("Connected to ");
                Serial.println(ssid);
                Serial.print("IP address: ");
                Serial.println(WiFi.localIP());

                // Setup MDNS responder
                if (!MDNS.begin(host.c_str())) {
                    Serial.println("Error setting up MDNS responder!");
                    //TODO - maybe reboot here...?
                }
                else {
                    Serial.println("mDNS responder started");
                    Serial.println("http://" + host + ".local");
                    // Add service to MDNS-SD
                    MDNS.addService("http", "tcp", 80);
                }
            }
            else if (s == WL_DISCONNECTED ||
                    s == WL_CONNECTION_LOST ||
                    s == WL_CONNECT_FAILED ||
                    s == WL_NO_SSID_AVAIL ||
                    s == WL_IDLE_STATUS) {
                if (!InAPMode) {
                    Serial.println("Configuring access point...");
                    EnableSoftAP();
                    Serial.println("In AP mode");
                }
                else {
                    Serial.println("Already in AP mode");
                }
            }
        }
        if (s == WL_CONNECTED) {
            isConnected = true;
            MDNS.update();
        }
    }

    void set_on_connect(void (*callback)(void)) {
        on_connect_fn = callback;
    }

    void set_on_disconnect(void (*callback)(void)) {
        on_disconnect_fn = callback;
    }

    void call_on_connect() {
        if (on_connect_fn)
            (*on_connect_fn)();
    }

    void call_on_disconnect() {
        if (on_disconnect_fn)
            (*on_disconnect_fn)();
    }

};