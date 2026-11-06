#include "M5Unified.h"

// Include required for the ENV III module
#include <M5UnitENV.h>

// MQTT includes and constants
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <string.h>

const char* ssid = "Kyle’s IPhone";
const char* password = "yesyes123";
std::string mqtt_username = "kmurawsky0383";
std::string mqtt_password = "0383";
const char *mqtt_broker = "p07da41d.ala.us-east-1.emqxsl.com";

const int mqtt_port = 8883;

const std::string mqtt_base = "SENG3030/Wednesday/" + mqtt_username + "/DisplayDevice/";
const std::string acqusition_topic = "SENG3030/Wednesday/tkosatkina4510/AcquisitionDevice/#";
const std::string control_topic = "SENG3030/Wednesday/apalmer2583/DeviceController/#";

const std::string mqtt_battery = mqtt_base + "battery";

const std::string mqtt_bmp_temperature = mqtt_base + "qmp/temperature";
const std::string mqtt_bmp_pressure = mqtt_base + "qmp/pressure";

const std::string mqtt_sht_temperature = mqtt_base + "sht30/temperature";
const std::string mqtt_sht_humidity = mqtt_base + "sht30/humidity";
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

const String status_request = "STATUS_REQUEST";

const int MENU_SIZE = 10;
bool menuEnabled[MENU_SIZE] = {
  true,  // Acquire Temperature
  true,  // Acquire Humidity
  true, // Acquire Acceleration
  true,  // Acquire Altitude
  true,  // Acquire Battery Level
  true,  // Display Temperature
  true,  // Display Humidity
  true, // Display Acceleration
  true,  // Display Altitude
  true   // Display Battery Level
};

// I2C Pins for the external I2C port
#define EXT_SDA 32
#define EXT_SCL 33

// SHT30 Humidity and Temperature I2C sensor
SHT3X sht3x;
// QMP6988 Pressure and Temperature I2C sensor
QMP6988 qmp;
// Flag that ENV was set up
bool env_ok = false;

void setup_unit_env(void);
void read_unit_env(void);
void read_button_data(void);
void read_imu(void);
void speaker(void);
void setup_basic_display(void);
void setup_wifi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void sendDisplayState(void);

void setup()
{
  //
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  Serial.println("** Starting Setup **");

  // Set up the ENV Sensor
  setup_unit_env();

  // Set up the display
  setup_basic_display();

  // MQTT setup
  setup_wifi();

  // Set Root CA certificate
  espClient.setCACert(ca_cert);

  client.setServer(mqtt_broker, mqtt_port);
  client.setKeepAlive(60);
  client.setCallback(mqttCallback);
  connectToMQTTBroker();

}

void loop()
{
  //Serial.println("** Running Loop **");
  //read_unit_env();
  //read_button_data();

  // Also see the draw functions in the 
  // Display example at M5Stack 

  delay(1000);

  if (!client.connected()) {
        connectToMQTTBroker();
  }
  // Required for subscriptions to fire callback
  client.loop();

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

            client.subscribe(acqusition_topic.c_str());
            client.subscribe(control_topic.c_str());
            client.publish(mqtt_base.c_str(), status_request.c_str());
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(25000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);

    String message = "";

    Serial.print("Message:");
    for (unsigned int i = 0; i < length; i++) {
        //Serial.print((char) payload[i]);
        message += (char) payload[i];
    }
    Serial.println(message);
    Serial.println("-----------------------");

    // A4
    String topicStr = String(topic);

    // Check which device is sending a message
    if (topicStr.indexOf("DeviceController") >= 0) {
      // Check to make sure the message is for our device D
      if (message[0] == 'D')
      {
        // Parse and change display appropraitely 

      }
      else if (message == status_request)
      {
        // Send current display state
        sendDisplayState();
      }
    } // Else the message is from the AcquisitionDevice
    // Check to make sure 
    // Parse and display sensor info

}

void sendDisplayState(void)
{
  //if (client.connected())

    String message = "Stuff";
    client.publish(mqtt_base.c_str(), message.c_str());
}


void setup_basic_display(void)
{
  M5.Display.clear();
  M5.Display.setRotation(1);
  M5.Display.setTextColor(GREEN);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(0, 40);
  M5.Display.printf("Example Number: %d\r\n", 10);

}

void setup_unit_env()
{
  Serial.println("** Setting Up Unit ENV **");

  if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, EXT_SDA, EXT_SCL, 400000U))
  {
    Serial.println("Couldn't find QMP6988");
    while (1)
      delay(1);
  }

  if (!sht3x.begin(&Wire, SHT3X_I2C_ADDR, EXT_SDA, EXT_SCL, 400000U))
  {
    Serial.println("Couldn't find SHT3X");
    while (1)
      delay(1);
  }

  env_ok = true;
}

void read_unit_env(void)
{
  if (not env_ok)
  {
    Serial.println("**** Unit ENV not set up ****");
    return;
  }

  if (sht3x.update())
  {
    Serial.println("-----SHT3X-----");
    Serial.print("Temperature: ");
    Serial.print(sht3x.cTemp);
    Serial.println(" degrees C");
    Serial.print("Humidity: ");
    Serial.print(sht3x.humidity);
    Serial.println("% rH");
    Serial.println("-------------\r\n");
  }

  if (qmp.update())
  {
    Serial.println("-----QMP6988-----");
    Serial.print(F("Temperature: "));
    Serial.print(qmp.cTemp);
    Serial.println(" *C");
    Serial.print(F("Pressure: "));
    Serial.print(qmp.pressure);
    Serial.println(" Pa");
    Serial.print(F("Approx altitude: "));
    Serial.print(qmp.altitude);
    Serial.println(" m");
    Serial.println("-------------\r\n");
  }

}

void read_button_data(void)
{
  M5.update();  // Required to get states
  if (M5.BtnA.wasPressed()) {
      Serial.println("A Btn Pressed");
      // Read and print env sensors
      read_unit_env();

      // Initialize and print and integer that holds the returned voltage in mV
      int voltage = M5.Power.getBatteryVoltage();
      Serial.print("Battery Voltage: ");
      Serial.println(voltage);
  }
  if (M5.BtnA.wasReleased()) {

      Serial.println("A Btn Released");
  }
  if (M5.BtnB.wasPressed()) {
      Serial.println("B Btn Pressed");
      // Read and print imu sensors
      read_imu();
  }
  if (M5.BtnB.wasReleased()) {
      Serial.println("B Btn Released");
  }
  if (M5.BtnPWR.wasClicked()) {
      Serial.println("PWR Btn Clicked");
      // Make beep sound
      speaker();
  }
  if (M5.BtnPWR.wasHold()) {
      Serial.println("PWR Btn Hold");
  }
}

void read_imu(void) 
{
  auto imu_update = M5.Imu.update();
  if (imu_update)
  {

    auto data = M5.Imu.getImuData();

    // The data obtained by getImuData can be used as follows.
    // data.accel.x;      // accel x-axis value.
    // data.accel.y;      // accel y-axis value.
    // data.accel.z;      // accel z-axis value.
    // data.accel.value;  // accel 3values array [0]=x / [1]=y / [2]=z.
    Serial.println("-----Accelerometer-----");
    Serial.print("X: ");
    Serial.println(data.accel.x);
    Serial.print("Y: ");
    Serial.println(data.accel.y);
    Serial.print("Z: ");
    Serial.println(data.accel.z);

    // data.gyro.x;      // gyro x-axis value.
    // data.gyro.y;      // gyro y-axis value.
    // data.gyro.z;      // gyro z-axis value.
    // data.gyro.value;  // gyro 3values array [0]=x / [1]=y / [2]=z.
    Serial.println("-----Gyroscope-----");
    Serial.print("X: ");
    Serial.println(data.gyro.x);
    Serial.print("Y: ");
    Serial.println(data.gyro.y);
    Serial.print("Z: ");
    Serial.println(data.gyro.z);

    // data.value;  // all sensor 9values array [0~2]=accel / [3~5]=gyro /
    //              // [6~8]=mag
  }
  delay(100); // CRITICAL – can’t read this too quickly or too slowly
}

void speaker(void) 
{
  Serial.println("Beep!");
  M5.Speaker.tone(4000, 20);
}