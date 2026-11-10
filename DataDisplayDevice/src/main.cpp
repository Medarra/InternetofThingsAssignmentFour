#include "M5Unified.h"

// Include required for the ENV III module
#include <M5UnitENV.h>

// MQTT includes and constants
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char *ssid = "Kyle’s IPhone";
const char *password = "yesyes123";
std::string mqtt_username = "kmurawsky0383";
std::string mqtt_password = "0383";
const char *mqtt_broker = "p07da41d.ala.us-east-1.emqxsl.com";

const int mqtt_port = 8883;

const std::string mqtt_base = "SENG3030/Wednesday/" + mqtt_username + "/DisplayDevice/";
const std::string acqusition_topic = "SENG3030/Wednesday/tkosatkina4510/AcquisitionDevice/#";
const std::string control_topic = "SENG3030/Wednesday/apalmer2583/DeviceController/#";

WiFiClientSecure espClient;
PubSubClient client(espClient);

char mqtt_buffer[40];

const char *ca_cert = R"EOF(
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

const int MENU_SIZE = 5; // THALB
const int AXIS = 3;      // x, y, z

bool menuEnabled[MENU_SIZE] = {
    true, // Display Temperature
    true, // Display Humidity
    true, // Display Acceleration
    true, // Display Altitude
    true  // Display Battery Level
};

int selectedPage = 0;

double sensors[MENU_SIZE] = {0};
double accelerometer[AXIS] = {0};

// Function Prototypes
void read_button_data(void);
void speaker(void);
void setup_wifi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);
void sendDisplayState(void);
void checkPage(void);
void drawMenu(void);
void updateDisplayState(String message);
void parseSensorData(String message);

void setup()
{
  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  Serial.println("** Starting Setup **");

  // Set up the display
  M5.Display.setRotation(1);
  M5.Display.setTextColor(GREEN);
  M5.Display.setTextDatum(middle_center);
  M5.Display.setFont(&fonts::FreeSansBold9pt7b);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(0, 40);
  drawMenu();

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
  // Continously read for button clicks
  read_button_data();

  delay(100);

  if (!client.connected())
  {
    connectToMQTTBroker();
  }
  // Required for subscriptions to fire callback
  client.loop();
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectToMQTTBroker()
{
  while (!client.connected())
  {
    String client_id = "esp32-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username.c_str(), mqtt_password.c_str()))
    {
      Serial.println("Connected to MQTT broker");

      // Subscribe to the Acquisition Device (Tatiana), Controller Device (Aly) and publish out our status request since have started running
      client.subscribe(acqusition_topic.c_str());
      client.subscribe(control_topic.c_str());
      client.publish(mqtt_base.c_str(), status_request.c_str());
    }
    else
    {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(25000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  String message = "";

  Serial.print("Message:");
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }
  Serial.println(message);
  Serial.println("-----------------------");

  // A4
  String topicStr = String(topic);

  // Check which device is sending a message
  if (topicStr.indexOf("DeviceController") >= 0)
  {
    // Check to make sure the message is for our device D and its either the status to sync or a 3 char toggle command
    if (message[0] == 'D' && 
      (message.length() == 6 || message.length() == 3))
    {
      // Parse and update menuEnabled values appropraitely
      updateDisplayState(message);

      // Swap off of current display if current display's sensor was just disabled
      if (selectedPage != 0 && !menuEnabled[selectedPage - 1])
      {
        checkPage();
        drawMenu();
      }
    }
    else if (message == status_request)
    {
      // Send current display state
      sendDisplayState();
    } // Else DeviceController message is for the AcquisitionDevice and we don't need to do anything
  } // Else the message is from the AcquisitionDevice to use and is larger than a 1 char string - Make sure its the readings and not the status message or request
  else if (topicStr.indexOf("AcquisitionDevice") >= 0 &&
           message != status_request &&              // Make sure its not a status request
           message.length() > 1 &&                   // Make sure the length is greater than 1
           (message[1] != 'T' && message[1] != 'F')) // Make sure its not the acquisition's current state
  {
    Serial.println("Acquisition Device sent sensor data");
    Serial.println("-----------------------");

    // Parse and save sensor data - redraw pages/menu since values may have been updated
    parseSensorData(message);
    drawMenu();
  }
}

void parseSensorData(String message)
{
  // Declare and initialize variables to hold the type of sensor and its reading
  char sensor = message[0];
  double reading = message.substring(1).toDouble();

  // Switch case for every type of sensor
  switch (sensor)
  {
  case 'T': // Store temperature
    sensors[0] = reading;
    break;

  case 'H': // Store humidity
    sensors[1] = reading;
    break;

  case 'A': // Store accelerometers 3 axis values
  {
    // Parse the 3 axis values by finding the commas
    int firstComma = message.indexOf(',');
    int secondComma = message.indexOf(',', firstComma + 1);

    // Check if indexOf failed to find the commas
    if (firstComma == -1 || secondComma == -1)
    {
      Serial.println("Invalid accelerometer data");
      break;
    }

    // Assign xyz values
    accelerometer[0] = message.substring(1, firstComma).toDouble();
    accelerometer[1] = message.substring(firstComma + 1, secondComma).toDouble();
    accelerometer[2] = message.substring(secondComma + 1).toDouble();

    break;
  }

  case 'L': // Store altitude
    sensors[3] = reading;
    break;

  case 'B': // Store battery
    sensors[4] = reading;
    break;

  default:
    break;
  }
}

void sendDisplayState(void)
{
  String message = "D";

  for (int i = 0; i < 5; i++)
  {
    message += menuEnabled[i] ? "T" : "F";
  }

  Serial.print("Publishing Display Status: ");
  Serial.println(message);

  client.publish(mqtt_base.c_str(), message.c_str());
}

void read_button_data(void)
{
  M5.update(); // Required to get states
  if (M5.BtnA.wasPressed())
  {
    Serial.println("A Btn Pressed");
    // Check what page we should be displaying and then draw it
    checkPage();
    drawMenu();
    // Make beep sound for effect
    speaker();
  }
}

void updateDisplayState(String message)
{
  if (message.length() == 6)
  {
    for (int i = 0; i < MENU_SIZE; i++)
    {
      menuEnabled[i] =
          (message[i + 1] == 'T');
    }
  }
  else if (message.length() == 3)
  {
    // Declare and initialize a String to hold our predefined sensor order
    String sensorOrder = "THALB";

    int index = sensorOrder.indexOf(message[1]);

    // Check to make sure indexOf returned a valid index and that our comand from the controller is a true or false
    if (index >= 0 && index < MENU_SIZE &&
        (message[2] == 'T' || message[2] == 'F'))
    {
      // Toggle that menu/sensor
      menuEnabled[index] =
          (message[2] == 'T');
    }
  }
}

void checkPage(void)
{
  // Page through displays
  selectedPage++;

  // Wrap around displays
  if (selectedPage > MENU_SIZE)
  {
    selectedPage = 0;
  }

  // While we are not on the opening page and sensor is disabled advance again and wrap if needed
  while (selectedPage != 0 && !menuEnabled[selectedPage - 1])
  {
    selectedPage++;
    if (selectedPage > MENU_SIZE)
    {
      selectedPage = 0;
    }
  }
}

void drawMenu()
{
  M5.Display.clear();
  M5.Display.setCursor(0, 40);

  switch (selectedPage)
  {
  case 0:
    // display page 0
    M5.Display.printf("Reading Sensors From: %s\r\n", acqusition_topic.c_str());
    break;

  case 1:
    // display page 1 Temperature
    M5.Display.printf("Temperature: %.2f C\r\n", sensors[0]);
    break;

  case 2:
    // display page 2 Humidity
    M5.Display.printf("Humidity: %.2f %%\r\n", sensors[1]);
    break;

  case 3:
    // display page 3 Accelerometer
    M5.Display.printf("Accelerometer: \r\n");
    M5.Display.printf("X: %.3f\r\n", accelerometer[0]);
    M5.Display.printf("Y: %.3f\r\n", accelerometer[1]);
    M5.Display.printf("Z: %.3f\r\n", accelerometer[2]);
    break;

  case 4:
    // display page 4 Altitude
    M5.Display.printf("Altitude: %.2f m\r\n", sensors[3]);
    break;

  case 5:
    // display page 5 Battery
    M5.Display.printf("Battery: %.0f mV\r\n", sensors[4]);
    break;

  default:
    break;
  }
}

void speaker(void)
{
  Serial.println("Beep!");
  M5.Speaker.tone(4000, 20);
}