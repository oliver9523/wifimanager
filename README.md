# wifimanager
wifi manager for ESP8266 (and I assume similar ESP32 but untested) devices to handle switching between AP/client mode

Mostly reused code from https://github.com/esp8266/Arduino/blob/master/libraries/DNSServer/examples/CaptivePortalAdvanced/CaptivePortalAdvanced.ino
but wrapped it in a class to make it nicer to use in my projects.

The WiFiManager class will enable the soft AP on the device when the host wifi connection is lost. 
The aim is to enable devices to be reachable if you change your main wifi settings and the IoT device drops off the network you can connect directly too it and update the wifi credentials which are then stored in the EEPROM for retrieval after rebooting. 

Typical serial monitor output when a wifi host cannot be reached

```
Booting...
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
WiFi failed, retrying.
Wifi status changed: Disconnected
Configuring access point...
In AP mode
Wifi status changed: No SSID
Already in AP mode
Wifi status changed: Disconnected
Already in AP mode
Wifi status changed: No SSID
Already in AP mode
```

Example of the device in AP mode

<img src="images/android_wifi.jpg" alt="android wifi ap" width="300"/>


Connect to it and navigate to `http://172.16.0.1/wifi` to update the wifi credentials

<img src="images/ap_wifi_config.jpg" alt="ap config update" width="300"/>


If this was useful and has saved you a few hours and you want to say thanks feel free to buy-me-a-coffee!
<a href="https://www.buymeacoffee.com/manythanks" target="_blank"><img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee" style="height: 41px !important;width: 174px !important;box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;-webkit-box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;" ></a>
