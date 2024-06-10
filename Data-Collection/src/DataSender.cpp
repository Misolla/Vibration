#include "DataSender.h"

DataSender::DataSender(String userId, String location, String serverUrl)
: _userId(userId), _location(location), _serverUrl(serverUrl) {
    Serial.println("[DataSender] Initialized with user ID, location, and server URL");
}

void DataSender::connectToWiFi(String ssid, String password) {
  Serial.println("[WiFi] Connecting to WiFi...");
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Connected to WiFi.");
}

void DataSender::connectToWPAEnterprise(String ssid, String username, String password) {
  Serial.println("[WiFi] Connecting to WPA Enterprise...");

  WiFi.disconnect(true);  // Disconnect from any network
  WiFi.mode(WIFI_STA); // Set WiFi to Station Mode

  // Initialize the WPA2 Enterprise parameters
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username.c_str(), username.length());
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username.c_str(), username.length());
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password.c_str(), password.length());
  
  // Enable WPA2 Enterprise
  esp_wifi_sta_wpa2_ent_enable();

  WiFi.begin(ssid.c_str()); // Start connection attempt

  // Wait for connection result
  Serial.print("Waiting for connection...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WiFi] Connected to WPA Enterprise successfully!");

  // Optional: Manually set DNS servers if DHCP does not work correctly
  ip_addr_t dnsserver;
  IP_ADDR4(&dnsserver, 8, 8, 8, 8); // Google's DNS as an example
  dns_setserver(0, &dnsserver);
}

void DataSender::sendData(String sensorType, String sensorName, float value, String unit) {
  Serial.println("[DataSender] Preparing to send data...");
  // Store the last data entry
  _lastData = "{ \"auth\": \"" + _userId +
                        "\", \"location\": \"" + _location +
                        "\", \"sensorType\": \"" + sensorType +
                        "\", \"sensorName\": \"" + sensorName +
                        "\", \"value\": " + String(value, 2) +
                        ", \"unit\": \"" + unit + "\" }";
  _readyToSend = true; // Mark as ready to send
  Serial.println("[DataSender] Data stored and marked as ready to send.");

  if (_checkRateLimit() && _readyToSend) {
    Serial.println("[DataSender] Rate limit check passed, attempting to send data...");
    _performSend();
  } else {
    Serial.println("[DataSender] Rate limit not passed or not ready to send.");
  }
}

bool DataSender::_checkRateLimit() {
  unsigned long currentTime = millis();

  if (currentTime - _lastSendTime < 60000) { // Less than a minute
    Serial.println("[DataSender] Rate limit in effect, waiting...");
    return false; // Don't send data, rate limit in effect
  }
  Serial.println("[DataSender] Rate limit passed.");
  _lastSendTime = currentTime; // Update last send time
  return true;
}

void DataSender::_performSend() {
  if (!_readyToSend) {
    Serial.println("[DataSender] No data ready to send.");
    return; // Nothing to send if not ready
  }

  // Check if server URL is valid
  if (_serverUrl.length() == 0) {
    Serial.println("[DataSender] Server URL is empty.");
    return;
  }

  HTTPClient http;
  Serial.println("[DataSender] Initializing HTTPClient...");

  Serial.println("[DataSender] Serialized JSON data to send: " + _lastData);

  http.begin(_serverUrl.c_str()); // Specify request destination
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(_lastData); // Send the request
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("[DataSender] Data sent successfully, server response: " + response);
  } else {
    Serial.println("[DataSender] Error on sending POST: " + String(httpResponseCode));
  }

  http.end(); //Close connection
  _readyToSend = false; // Reset ready to send flag after sending
}

void DataSender::sendLocalTime() {
    // Configure NTP server and timezone
    const char* ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = -28800; // Set your GMT offset in seconds (example is for PST, GMT-8)
    const int daylightOffset_sec = 3600; // Set daylight offset in seconds

    // Initialize time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeInfo;
    if (!getLocalTime(&timeInfo)) {
        Serial.println("[DataSender] Failed to obtain time");
        return;
    }

    // Format the time as a string
    char timeStringBuff[50];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeInfo);
    _localTime = String(timeStringBuff);  // Store the formatted time

    // Prepare the JSON data
    String jsonData = "{ \"auth\": \"" + _userId +
                      "\", \"location\": \"" + _location +
                      "\", \"localTime\": \"" + _localTime + "\" }";

    // Send the data
    HTTPClient http;
    http.begin(_serverUrl.c_str()); // Specify request destination
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData); // Send the request
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("[DataSender] Time data sent successfully, server response: " + response);
    } else {
        Serial.println("[DataSender] Error on sending POST: " + String(httpResponseCode));
    }

    http.end(); //Close connection
}

String DataSender::getTime() {
    return _localTime;
}

String DataSender::getTimeOnly() {
    if (_localTime.length() > 15) { // Assuming the format "YYYY-MM-DD HH:MM:SS"
        return _localTime.substring(11,16); // Extract the time portion
    }
    return ""; // Return an empty string if the format is unexpected
}

