#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Initialize web server on port 80
ESP8266WebServer server(80);

// WiFi credentials and device ID
String ssid = "NodeMCU-AP", password = "";
String content;
String deviceId = String(ESP.getFlashChipId());

const int relayPin = D5;   // GPIO pin connected to the relay
bool relayStatus = false;  // Variable to store relay status (OFF by default)

// WiFi connection attempt parameters
const int maxConnectionAttempts = 10; // Maximum number of connection attempts
const int connectionAttemptInterval = 500; // Interval between attempts (milliseconds)

void setup() {
  // Start serial communication
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(100);
  // Initialize relay pin
  pinMode(relayPin, OUTPUT);    // Set relay pin as output
  digitalWrite(relayPin, LOW);  // Ensure relay is initially off
  // Read stored data from EEPROM
  readData();
  // Set up Access Point mode
  const char* ssidap = "NodeMCU-AP";
  const char* passap = "";
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidap, passap);
  Serial.print("AP mode - http://");
  Serial.println(WiFi.softAPIP());
  // Try to connect to the WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < maxConnectionAttempts) {
    delay(connectionAttemptInterval);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Failed to connect to WiFi. Continuing in AP mode.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidap, passap);
    Serial.print("AP mode - http://");
    Serial.println(WiFi.softAPIP());
  }
  // Launch the web server
  launchWeb();
}

void launchWeb() {
  createWebServer();
  server.begin();
}

void loop() {
  server.handleClient();
}

// Function to create the web server routes
void createWebServer() {
  // Main page route
  server.on("/", []() {
    String content = "<!DOCTYPE HTML>\r\n<html><head>";
    content += "<style>";
    content += "body { font-family: Arial, sans-serif; background-color: #f2f2f2; text-align: center; }";
    content += "h1 { color: #333; }";
    content += "h2 { color: #666; }";
    content += "form { margin: 20px auto; padding: 20px; background: #fff; border: 1px solid #ccc; border-radius: 10px; width: 300px; }";
    content += "label { display: block; margin-bottom: 10px; }";
    content += "input[type='text'], input[type='password'] { width: 80%; padding: 10px; margin-bottom: 10px; border: 1px solid #ccc; border-radius: 5px; }";
    content += "input[type='submit'] { padding: 10px 20px; background: #28a745; border: none; border-radius: 5px; color: white; cursor: pointer; }";
    content += "input[type='submit']:hover { background: #218838; }";
    content += ".switch { position: relative; display: inline-block; width: 60px; height: 34px; }";
    content += ".switch input { opacity: 0; width: 0; height: 0; }";
    content += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; transition: .4s; border-radius: 34px; }";
    content += ".slider:before { position: absolute; content: ''; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; transition: .4s; border-radius: 50%; }";
    content += "input:checked + .slider { background-color: #2196F3; }";
    content += "input:checked + .slider:before { transform: translateX(26px); }";
    content += ".switch-label { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }";
    content += "</style></head><body>";

    content += "<h1>Access Point Mode</h1><br>";
    content += "<h2>Welcome to NodeMCU WiFi configuration</h2>";
    content += "<p><b>Your current configuration</b></p>";
    content += "SSID: " + ssid + "<br>";
    content += "Password: " + password + "<br>";
    content += "Device ID: " + deviceId + "<br>";
    content += "<form method='get' action='setting'>";
    content += "<label>SSID: </label><input type='text' name='ssid' length=32><br>";
    content += "<label>Password:</label><input type='password' name='password' length=32><br>";
    content += "<label>Device ID:</label><input type='text' name='deviceId' length=32><br>";
    content += "<input type='submit' value='Save Configuration'></form>";
    // Relay control section with toggle switch
    content += "<h2>Relay Control</h2>";
    content += "<form method='get' action='relay'>";
    content += "<div class='switch-label'><span>OFF</span><label class='switch'><input type='checkbox' name='relayStatus' value='ON'" + String(relayStatus ? " checked" : "") + "><span class='slider'></span></label><span>ON</span></div>";
    content += "<input type='submit' value='Set Relay Status'></form>";
    content += "</body></html>";
    // Send the HTML content to the client
    server.send(200, "text/html", content);
  });
  // Route to handle configuration settings
  server.on("/setting", []() {
    // Update WiFi credentials and device ID from form data
    ssid = server.arg("ssid");
    password = server.arg("password");
    deviceId = server.arg("deviceId");
    // Save the new data to EEPROM
    writeData(ssid, password, deviceId);
    String content = "<!DOCTYPE HTML>\r\n<html><head>";
    content += "<style>";
    content += "body { font-family: Arial, sans-serif; background-color: #f2f2f2; text-align: center; }";
    content += "h1 { color: #333; }";
    content += "p { color: #666; }";
    content += ".container { margin: 20px auto; padding: 20px; background: #fff; border: 1px solid #ccc; border-radius: 10px; width: 300px; }";
    content += "a { text-decoration: none; color: #fff; background: #28a745; padding: 10px 20px; border-radius: 5px; }";
    content += "a:hover { background: #218838; }";
    content += "</style></head><body>";
    content += "<div class='container'>";
    content += "<h1>Configuration Saved</h1>";
    content += "<p>Success. Please back to take effect.</p>";
    content += "<a href='/'>Back</a>";
    content += "</div>";
    content += "</body></html>";
    server.send(200, "text/html", content);
  });
  // Route to handle relay control
  server.on("/relay", []() {
    // Check the status of the relay switch
    if (server.hasArg("relayStatus")) {
      relayStatus = true;  // Turn relay ON
    } else {
      relayStatus = false;  // Turn relay OFF
    }
    // Update the relay pin accordingly
    digitalWrite(relayPin, relayStatus ? HIGH : LOW);  // Set relay according to status
    String content = "<!DOCTYPE HTML>\r\n<html><head>";
    content += "<style>";
    content += "body { font-family: Arial, sans-serif; background-color: #f2f2f2; text-align: center; }";
    content += "h1 { color: #333; }";
    content += "p { color: #666; }";
    content += ".container { margin: 20px auto; padding: 20px; background: #fff; border: 1px solid #ccc; border-radius: 10px; width: 300px; }";
    content += "a { text-decoration: none; color: #fff; background: #007bff; padding: 10px 20px; border-radius: 5px; }";
    content += "a:hover { background: #0056b3; }";
    content += "</style></head><body>";
    content += "<div class='container'>";
    content += "<h1>Relay Status</h1>";
    content += "<p>Relay is now " + String(relayStatus ? "ON" : "OFF") + "</p>";
    content += "<a href='/'>Back</a>";
    content += "</div>";
    content += "</body></html>";
    server.send(200, "text/html", content);
  });
}
// Function to write data to EEPROM
void writeData(String a, String b, String c) {
  Serial.println("Writing to EEPROM");
  for (int i = 0; i < 20; i++) {
    EEPROM.write(i, i < a.length() ? a[i] : 0);
  }
  for (int i = 20; i < 40; i++) {
    EEPROM.write(i, i - 20 < b.length() ? b[i - 20] : 0);
  }
  for (int i = 40; i < 60; i++) {
    EEPROM.write(i, i - 40 < c.length() ? c[i - 40] : 0);
  }
  EEPROM.commit();
  Serial.println("Write successful");
}
// Function to read data from EEPROM
void readData() {
  Serial.println("Reading from EEPROM....");
  ssid = "";
  password = "";
  deviceId = "";
  for (int i = 0; i < 20; i++) {
    ssid += char(EEPROM.read(i));
  }
  for (int i = 20; i < 40; i++) {
    password += char(EEPROM.read(i));
  }
  for (int i = 40; i < 60; i++) {
    deviceId += char(EEPROM.read(i));
  }
  ssid.trim();
  password.trim();
  deviceId.trim();
  Serial.println("WiFi SSID from EEPROM: " + ssid);
  Serial.println("WiFi password from EEPROM: " + password);
  Serial.println("Device ID from EEPROM: " + deviceId);
  Serial.println("Reading successful.....");
}
