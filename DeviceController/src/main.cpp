#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <string.h>
#include "M5Unified.h"
// Include required for the ENV IV module
#include <M5UnitENV.h>

const char* ssid = "MedarraBellis";
const char* password = "20MoonstripE06";
std::string mqtt_username = "apalmer2583";
std::string mqtt_password = "2583";
const char *mqtt_broker = "p07da41d.ala.us-east-1.emqxsl.com";

const int mqtt_port = 8883;


const std::string mqtt_base = "SENG3030/Wednesday/" + mqtt_username + "/DeviceController/";
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

// I2C Pins for the external I2C port
#define EXT_SDA 32
#define EXT_SCL 33

// SHT30 Humidity and Temperature I2C sensor
SHT3X sht3x;
// QMP6988 Pressure and Temperature I2C sensor
QMP6988 qmp;
// Flag that ENV was set up
bool env_ok = false;


// Function Prototypes:
void setup_unit_env(void);
void read_unit_env(void);
void read_button_data(void);
void setup_wifi(void);
void connectToMQTTBroker(void);
void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  Serial.println("** Starting Setup **");

  // Set up the ENV Sensor
  setup_unit_env();

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
  Serial.println("** Running Loop **");
  read_unit_env();
  read_button_data();
  delay(1000);

  if (!client.connected()) {
      connectToMQTTBroker();
  }
  // Required for subscriptions to fire callback
  client.loop();
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
            client.subscribe((mqtt_base + "#").c_str());
            // Publish message upon successful connection
            client.publish((mqtt_base + "hello").c_str(), "Hello SENG3030!");
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
    Serial.print("Message:");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
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
  M5.update(); // Required to get states
  if (M5.BtnA.wasPressed()) {
  Serial.println("A Btn Pressed");
  }
  if (M5.BtnA.wasReleased()) {
  Serial.println("A Btn Released");
  }
  if (M5.BtnB.wasPressed()) {
  Serial.println("B Btn Pressed");
  }
  if (M5.BtnB.wasReleased()) {
  Serial.println("B Btn Released");
  }
  if (M5.BtnPWR.wasClicked()) {
  Serial.println("PWR Btn Clicked");
  }
  if (M5.BtnPWR.wasHold()) {
  Serial.println("PWR Btn Hold");
  }
}