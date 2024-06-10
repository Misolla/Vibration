#ifndef DataSender_h
#define DataSender_h

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_wpa2.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wpa2.h" // for WPA2 Enterprise
#include <lwip/dns.h> // for DNS


/**
 * @brief This is the class to send the data over wifi to the server. 
 * 
 * This class needs to be initialized with the user id, location, and server url.
 * These parameters will be saved and used to send the data to the server.
 * 
 * You can either connect to a regular wifi network or a WPA Enterprise network.
 */
class DataSender {
public:
  /**
   * @brief Construct a new DataSender object
   * 
   * @param userId The user ID to send to the server
   * @param location The location of the ESP32
   * @param serverUrl The URL of the server to send data to
   */
  DataSender(String userId, String location, String serverUrl);

  /**
   * @brief Connect to a regular WiFi network
   * 
   * @param ssid The SSID of the WiFi network
   * @param password The password of the WiFi network
   */
  void connectToWiFi(String ssid, String password);

  /**
   * @brief Connect to a WPA Enterprise network
   * 
   * @param ssid The SSID of the WiFi network
   * @param username The username for WPA Enterprise
   * @param password The password for WPA Enterprise
   */
  void connectToWPAEnterprise(String ssid, String username, String password);

  /**
   * @brief Send the data to the server
   * 
   * @param sensorType The type of sensor (e.g. temperature, humidity, light, pressure)
   * @param sensorName The name of the sensor (e.g. DHT11, BMP180, etc.)
   * @param value The value of the sensor reading
   * @param unit The unit of the sensor reading (e.g. degrees Celsius, hPa, lux, etc.)
   */
  void sendData(String sensorType, String sensorName, float value, String unit);
  void sendLocalTime();
  String getTime();
  String getTimeOnly();

private:
  String _userId;
  String _location;
  String _serverUrl;
  String _lastData;
  String _localTime;

  unsigned long _lastSendTime = 60001; // Initialize to ensure the first send is allowed
  
  bool _readyToSend = false;
  bool _checkRateLimit();
  void _performSend();
};

#endif