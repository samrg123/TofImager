#pragma once

extern "C" {
    bool ICACHE_FLASH_ATTR wifi_softap_set_dhcps_offer_option(uint8 level, void* optarg);
}

#include <ESP8266WiFi.h>

#include "log.h"

class Wifi: public ESP8266WiFiClass {

public:

    static inline constexpr uint8 kDefaultAPChannel = 1;
    static inline constexpr uint8 kDefaultAPMaxConnections = 4;

    static inline IPAddress kDefaultIp         = IPAddress(192, 168,  4, 1);
    static inline IPAddress kDefaultGateway    = IPAddress(192, 168,  4, 1);
    static inline IPAddress kDefaultSubnetMask = IPAddress(255, 255, 255, 0);

    static const char* WiFiStatusString(wl_status_t status) {
        switch(status) {
            case WL_IDLE_STATUS:      return "WL_IDLE_STATUS";
            case WL_NO_SSID_AVAIL:    return "WL_NO_SSID_AVAIL";
            case WL_SCAN_COMPLETED:   return "WL_SCAN_COMPLETED";
            case WL_CONNECTED:        return "WL_CONNECTED";
            case WL_CONNECT_FAILED:   return "WL_CONNECT_FAILED";
            case WL_CONNECTION_LOST:  return "WL_CONNECTION_LOST";
            case WL_WRONG_PASSWORD:   return "WL_WRONG_PASSWORD";
            case WL_DISCONNECTED:     return "WL_DISCONNECTED";
            case WL_NO_SHIELD:        return "WL_NO_SHIELD";
            default:                  return "INVALID";
        }
    }

    void PrintWiFiStatus() {

        Log("WiFi Status: {\n"
            "\tMAC: %s\n"
            "\tStatus: %s\n"
            "\tSSID: %s\n"
            "\tChannel: %d\n"
            "\tRSSI: %d\n"
            "\tGateway: %s\n"
            "\tSubnetMask: %s\n"
            "\tIP: %s\n"
            "\tDNS: %s\n"
            "}",

            macAddress().c_str(),
            WiFiStatusString(status()),
            SSID().c_str(),
            channel(),
            RSSI(),
            gatewayIP().toString().c_str(),
            subnetMask().toString().c_str(),
            localIP().toString().c_str(),
            dnsIP().toString().c_str()
        );
    }

    void PrintAPStatus() {

        softap_config config = {};

        if(!wifi_softap_get_config(&config)) {
            Error("Failed to get AP config");
            return;
        }

        ip_info ipInfo;
        if(!wifi_get_ip_info(SOFTAP_IF, &ipInfo)) {
            Error("Failed to get AP ipInfo");
            return;
        }

        Log("AP Status: {\n"
            "\tSSID: %s\n"
            "\tPSK: %s\n"
            "\tHidden: %d\n"
            "\tChannel: %d\n"
            "\tConnections: %d/%d\n"
            "\tBeacon Interval: %d\n"
            "\tIP: %s\n"
            "\tGateway: %s\n"
            "\tSubnet: %s\n"
            "\tMAC: %s\n"
            "}",

            softAPSSID().c_str(),
            softAPPSK().c_str(),
            config.ssid_hidden,
            config.channel,
            softAPgetStationNum(), config.max_connection,
            config.beacon_interval,
            IPAddress(ipInfo.ip.addr).toString().c_str(),
            IPAddress(ipInfo.gw.addr).toString().c_str(),
            IPAddress(ipInfo.netmask.addr).toString().c_str(),
            softAPmacAddress().c_str()
        );
    }

    void Connect(const char* ssid, const char* password) {

        Log("Connecting to SSID: '%s'", ssid);

        begin(ssid, password);

        wl_status_t wifiStatus;
        while((wifiStatus = status()),
                wifiStatus != WL_CONNECTED &&
                wifiStatus != WL_NO_SHIELD &&
                wifiStatus != WL_CONNECT_FAILED &&
                wifiStatus != WL_WRONG_PASSWORD) {

            Warn("Failed to connect. status = %s | Retrying", WiFiStatusString(wifiStatus));
            delay(500);
        }

        PrintWiFiStatus();
    }

    void HostNetwork(const char* ssid, const char* password, bool hidden = false, 
                     uint8 maxConnections = kDefaultAPMaxConnections, uint8 channel = kDefaultAPChannel) {
 
        Log("Hosting AP '%s'", ssid);

        // // TODO: THIS IS SUPPOSED TO DISABLE DHCP - but function isn't included in lwip lib? Double check this!
        // uint8 mode = 0;
        // wifi_softap_set_dhcps_offer_option(OFFER_ROUTER, &mode);

        mode(WIFI_AP);

        // WiFiMode currentMode = (WiFiMode)wifi_get_opmode();
        // Log("CurrentMode: %d", currentMode);

        // TODO: THIS IS CAUSES WIFI TO FAIL TO START AP MODE WHEN C OPTIMIZER IS 03???
        // persistent(false);

        // TODO: configuring AP before init causes crash on connection ... why?
        while(!softAPConfig(kDefaultIp, kDefaultGateway, kDefaultSubnetMask)) {
            Warn("Failed to configure AP. Retrying...");
            delay(500);
        }

        while(!softAP(ssid, password, channel, hidden, maxConnections)) {
            Warn("Failed to enable AP. Retrying...");
            delay(500);
        }

        PrintAPStatus();
    }    

};
