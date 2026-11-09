#include "M5Unified.h"

// Include required for the ENV IV module
#include <M5UnitENV.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <string.h>

// I2C Pins for the external I2C port
#define EXT_SDA 32
#define EXT_SCL 33

const char* ssid = "iPhone Tatiana";
const char* password = "0976431!";
std::string mqtt_username = "tkosatkina4510";
std::string mqtt_password = "4510";
std::string mqtt_controller = "apalmer2583/DeviceController/";
const char *mqtt_broker = "p07da41d.ala.us-east-1.emqxsl.com";

const int mqtt_port = 8883;

// MQTT Topics
const std::string mqtt_base = "SENG3030/Wednesday/";
const std::string mqtt_myDevice = "SENG3030/Wednesday/" + mqtt_username + "/";
const std::string mqtt_data = mqtt_myDevice + "AcquisitionDevice/";

WiFiClientSecure espClient;
PubSubClient client(espClient);

char mqtt_buffer[40];

const char* ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

// SHT30 Humidity and Temperature I2C sensor
SHT3X sht3x;
// QMP6988 Pressure and Temperature I2C sensor
QMP6988 qmp;
// Sensors initialized flag
bool env_ok = false;

// Status request flag
const String status_request = "STATUS_REQUEST";

// Sensor reading intervals (in milliseconds)
const unsigned long READ_INTERVAL = 30000;

// Flag sensor publishing
bool send_temperature = true;
bool send_humidity = true;
bool send_accelerometer = true;
bool send_altitude = true;
bool send_battery = true;

void setup_unit_env(void);
void read_unit_env(void);
void battery_voltage(void);
void track_motion(void);
void setup_wifi(void);
void connectToMQTTBroker(void);
void mqttCallback(char *topic, byte *payload, unsigned int length);
void publishSensorDataToMQTT(char sensor, float value);
void sendStatusRequest(void);
void receiveStatusRequest(String command);

/**
 * Function to setup the M5Stack device, initialize sensors, connect to Wi-Fi, 
 * and set up MQTT communication.
 */
void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  Serial.println("** Starting Setup **");

  // Set up the ENV Sensor
  setup_unit_env();
  // Setup Wi-Fi
  setup_wifi();

  espClient.setCACert(ca_cert);
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqttCallback);
}

/**
 * Main loop function that continuously reads sensor data, checks for MQTT connection, 
 * and processes incoming MQTT messages.
 */
void loop()
{
  M5.update();

  // Store the time of the last sensor readings
  static unsigned long lastSensorRead = 0;

  // Reconnect to MQTT broker if disconnected
  if (!client.connected()) {
    connectToMQTTBroker();
  }

  // Process incoming MQTT messages
  client.loop();

  // Get current time since device started
  unsigned long currentTime = millis();

  // Read and publish sensor data
  if (currentTime - lastSensorRead >= READ_INTERVAL) {
    lastSensorRead = currentTime;
    read_unit_env();
    battery_voltage();
    track_motion();
  }
}

void setup_unit_env()
{
  Serial.println("** Setting Up Unit ENV **");

  // Initialize QMP6988 pressure/altitude sensor
  if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, EXT_SDA, EXT_SCL, 400000U))
  {
    Serial.println("Couldn't find QMP6988");
    while (1)
    delay(1);
  }

  // Initialize SHT3X temperature/humidity sensor
  if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, EXT_SDA, EXT_SCL, 400000U))
  {
    Serial.println("Couldn't find SHT3X");
    while (1)
    delay(1);
  }

  env_ok = true;
}

/**
 * Function to read data from the ENV sensors (SHT3X and QMP6988) and publish the readings to the MQTT broker.
 * It checks if the sensors are set up correctly, reads temperature, humidity, pressure, 
 * and altitude, and publishes the data to the corresponding MQTT topics if the respective flags are enabled.
 */
void read_unit_env(void)
{
  // Check if the ENV sensor was set up successfully
  if (!env_ok)
  {
    Serial.println("**** Unit ENV not set up ****");
    return;
  }

  // Read temperature and humidity from the SHT3X sensor
  if (send_temperature || send_humidity) {
    if (sht3x.update()) {
    // Publish temperature data if enabled
    if (send_temperature) {
      publishSensorDataToMQTT('T', sht3x.cTemp);
    }
    // Publish humidity data if enabled
    if (send_humidity) {
      publishSensorDataToMQTT('H', sht3x.humidity);
    }
  } else {
    Serial.println("Failed to read SHT3X sensor data");
  }
}
  

  // Read altitude from the QMP6988 sensor
  if (send_altitude) {
    if (qmp.update())
    {
      // Publish altitude data if enabled
      if (send_altitude) {
        publishSensorDataToMQTT('L', qmp.altitude);
      }
    } else {
      Serial.println("Failed to read QMP6988 sensor data");
    }
  }
}

/**
 * Function to read the battery voltage from the M5Stack device and print it to the Serial monitor.
 * It retrieves the battery voltage using the M5.Power.getBatteryVoltage() method and prints the value in volts.
 */
void battery_voltage(void)
{
  // Check if battery publishing is enabled
  if (!send_battery) {
    return;
  }

  float voltage = M5.Power.getBatteryVoltage();

  Serial.printf("Published: %.3f V\n", voltage);

  // Publish battery voltage to MQTT
  publishSensorDataToMQTT('B', voltage);
}

/**
 * Function to track motion using the IMU sensor on the M5Stack device.
 * It updates the IMU sensor readings and prints the accelerometer data (x, y, z) to the Serial monitor.
 */
void track_motion(void)
{
  // Check if accelerometer publishing is enabled
  if (!send_accelerometer) {
    return;
  }

  // Read IMU data
  if (M5.Imu.update()) {

    auto data = M5.Imu.getImuData();

    // Read acceleration on three axes
    float accel_x = data.accel.x;
    float accel_y = data.accel.y;
    float accel_z = data.accel.z;

    // Create JSON payload
    char buffer[256];

    snprintf(buffer, sizeof(buffer), "A%.3f,%.3f,%.3f", accel_x, accel_y, accel_z);

    // Publish accelerometer data
    if (client.connected()) {

      if (client.publish(mqtt_data.c_str(), buffer)) {
        Serial.printf("Published: %s\n", buffer);
      } else {
        Serial.println("Failed to publish accelerometer data");
      }
    }
  } else {
    Serial.println("Failed to read accelerometer data");
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectToMQTTBroker() {
    while (!client.connected()) {
        String client_id = "esp32-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        
        if (client.connect(client_id.c_str(), mqtt_username.c_str(), mqtt_password.c_str())) {
          Serial.println("Connected to MQTT broker");
          
          // Subscribe to the topic for receiving commands from the Controller Device
          client.subscribe((mqtt_base + mqtt_controller).c_str());
          Serial.printf("Subscribed to topic: %s\n", (mqtt_base + mqtt_controller).c_str());

          // Request the current status of the sensors from the Controller Device
          if (client.publish(mqtt_data.c_str(), status_request.c_str())) {
            Serial.println("Status request published");
          } else {
            Serial.println("Failed to publish status request");
          }

        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // Check if the topic matches the expected topic for this device  
  if (String(topic) != (mqtt_base + mqtt_controller).c_str()) {
    Serial.printf("Received message on unexpected topic: %s\n", topic);
    return;
  }

  // Convert received payload to a string and remove any leading/trailing whitespace
  String command = "";
  for (unsigned int i = 0; i < length; i++) {
      command += (char)payload[i];
  }

  command.trim(); 
  command.toUpperCase();

  Serial.printf("Received command: %s\n", command.c_str());

  // Check command length and corresponding device
  if (command == status_request) {
    sendStatusRequest();
    return;
  } 
  
  if (command.length() == 6) {
    receiveStatusRequest(command);
    return;
  }
  
  if (command.length() != 3) {
    Serial.println("Invalid command length. Expected 3 characters.");
    return;
  }

  char device = command[0];
  char sensor = command[1];
  char state = command[2];

  if (device != 'A') {
    Serial.println("Invalid device identifier. Expected 'A'.");
    return;
  }

  // Check whether the command is to turn on or off a sensor and execute the corresponding action
  if (state == 'T') {
    
    switch (sensor) {
      case 'T':
        send_temperature = true;
        Serial.println("Temperature sensor turned ON");
        break;
      case 'H':
        send_humidity = true;
        Serial.println("Humidity sensor turned ON");
        break;
      case 'A':
        send_accelerometer = true;
        Serial.println("Accelerometer sensor turned ON");
        break;
      case 'L':
        send_altitude = true;
        Serial.println("Altitude sensor turned ON");
        break;
      case 'B':
        send_battery = true;
        Serial.println("Battery sensor turned ON");
        break;
      default:
        Serial.println("Invalid sensor identifier.");
    }

  } else if (state == 'F') {
    
    switch (sensor) {
      case 'T':
        send_temperature = false;
        Serial.println("Temperature sensor turned OFF");
        break;
      case 'H':
        send_humidity = false;
        Serial.println("Humidity sensor turned OFF");
        break;
      case 'A':
        send_accelerometer = false;
        Serial.println("Accelerometer sensor turned OFF");
        break;
      case 'L':
        send_altitude = false;
        Serial.println("Altitude sensor turned OFF");
        break;
      case 'B':
        send_battery = false;
        Serial.println("Battery sensor turned OFF");
        break;
      default:
        Serial.println("Invalid sensor identifier.");
    } 
  } else {
  Serial.println("Invalid command state. Expected T or F.");
  return;
  }
}

/**
 * Function to send status request to the Controller Device on request
 */
void sendStatusRequest(void) {
  if (client.connected()) {
    String message = "";
    // Temperature/Humidity/Accellerometer/Altitude/Battery
    char device = 'A';
    char temp = send_temperature ? 'T' : 'F';
    char hum = send_humidity ? 'T' : 'F';
    char accel = send_accelerometer ? 'T' : 'F';
    char alt = send_altitude ? 'T' : 'F';
    char batt = send_battery ? 'T' : 'F';

    message = String(device) + temp + hum + accel + alt + batt;
  
    client.publish(mqtt_data.c_str(), message.c_str());
    Serial.println("Status request sent to Controller Device: " + message);
  } else {
    Serial.println("Cannot send status request. MQTT client not connected.");
  }
}

/**
 * Function to receive status request from the Controller Device
 */
void receiveStatusRequest(String command) {
  // Check status response
  if (command.length() != 6) {
    Serial.println("Invalid status response length.");
    return;
  }

  // Check if the first character is 'A' for Acquisition Device
  if (command[0] != 'A')
  {
      Serial.println("Not an acquisition status packet.");
      return;
  }

  // Check if the remaining characters are either 'T' or 'F' for each sensor state
  for (int i = 1; i < 6; i++) {
    if (command[i] != 'T' && command[i] != 'F') {
        Serial.println("Invalid sensor state in status response.");
        return;
    }
  }

  send_temperature = (command[1] == 'T');
  send_humidity = (command[2] == 'T');
  send_accelerometer = (command[3] == 'T');
  send_altitude = (command[4] == 'T');
  send_battery = (command[5] == 'T');

  Serial.println("Sensor states restored: " + command);
}

/**
 * Helping function to publish sensor data to the MQTT broker. 
 * It checks if the client is connected, converts the float value to a string, 
 * and publishes it to the specified topic. 
 */
void publishSensorDataToMQTT(char sensor, float value) {
  if (!client.connected()) {
    return;
  }

  // Convert the float value to a string and publish it to the MQTT topic
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%c%.2f", sensor, value);
  
  // Check if the publish was successful and print the result
  if (client.publish(mqtt_data.c_str(), buffer)) {
    Serial.printf("Published: %s\n", buffer);
  } else {
    Serial.println("Failed to publish sensor data");
  }
}