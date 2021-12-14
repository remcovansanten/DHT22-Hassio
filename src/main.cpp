/*****
Description: Simple program to publish temperature, humidity and heat index to home assistant (HASSIO)
Uses an ESP32 and DHT22 as components and MQTT / JSON for integration

Author: Remco van Santen
License: Public
----
Hassio configuration.yaml snippet:
sensor:
  - platform: mqtt
    name: "Office Temperature"
    unique_id: "OfficeTemp_1"
    state_topic: esp32/office
    unit_of_measurement: "°C"
    value_template: "{{ value_json.OfficeTemperature }}"
  - platform: mqtt
    name: "Office Humidity"
    unique_id: "Officehum_1"
    state_topic: esp32/office
    unit_of_measurement: "%"
    value_template: "{{ value_json.Officehumidity }}"
  - platform: mqtt
    name: "Office HeatIndex"
    unique_id: "OfficeHic_1"
    state_topic: esp32/office
    unit_of_measurement: "°C"
    value_template: "{{ value_json.OfficeheatIndex }}"
-----
arduino_secrets.h contents:
#define SECRET_SSID "your_ssid"
#define SECRET_PASS "yourwifipassword"
const char* secret_mqtt_username = "hassio_mqtt_user";
const char* secret_mqtt_password = "hassio_mqtt_password";
 -----
*****/

// Loading the ESP8266WiFi library and the PubSubClient library
#include <arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <ArduinoJson.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
#include "../include/arduino_secrets.h"
const char *ssid = SECRET_SSID;     // your network SSID (name)
const char *password = SECRET_PASS; // your network password
const char *mqtt_username = secret_mqtt_username;
const char *mqtt_password = secret_mqtt_password;

// Deepsleep options
#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  120        //Time ESP32 will go to sleep (in seconds)


// Uncomment one of the lines bellow for whatever DHT sensor type you're using!
//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Json array size
const int capacity = JSON_ARRAY_SIZE(3) + 2*JSON_OBJECT_SIZE(2);

// -----  MQTT server config -----
// Username/password in arduino_secrets.h
const PROGMEM char *mqtt_server = "homeassistant";
const PROGMEM char *MQTT_SENSOR_TOPIC = "esp32/office";
const PROGMEM uint16_t mqtt_port = 1883;

// Initializes the espClient
WiFiClient espClient;
PubSubClient client(espClient);

// Connect an LED to each GPIO of your ESP8266
const int ledGPIO5 = 5;
const int ledGPIO4 = 4;

// DHT Sensor
const int DHTPin = 15;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// Don't change the function below. This functions connects your ESP8266 to your router
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
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// This functions is executed when some device publishes a message to a topic that your ESP32 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic home/office/esp1/gpio2, you check if the message is either 1 or 0. Turns the ESP GPIO according to the message
  if(topic=="esp32office/4"){
      Serial.print("Changing GPIO 4 to ");
      if(messageTemp == "1"){
        digitalWrite(ledGPIO4, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "0"){
        digitalWrite(ledGPIO4, LOW);
        Serial.print("Off");
      }
  }
  if(topic=="esp32office/5"){
      Serial.print("Changing GPIO 5 to ");
      if(messageTemp == "1"){
        digitalWrite(ledGPIO5, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "0"){
        digitalWrite(ledGPIO5, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
/**
 * @brief ---- reconnect to MQTT server ----
 * automatic shutdown after 15 seconds as this is default
 */
void reconnect()
{
  while (!client.connected())
  {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Home assistant mqtt broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}


// The setup function sets your ESP GPIOs to Outputs, starts the serial communication at a baud rate of 115200
// Sets your mqtt broker and sets the callback function
// The callback function is what receives messages and actually controls the LEDs
void setup() {
  dht.begin();
  pinMode(ledGPIO4, OUTPUT);
  pinMode(ledGPIO5, OUTPUT);
  
  Serial.begin(115200);

  //Set timer to timer length
	esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
	Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
	" Seconds");

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.loop();
}

/**
 * @brief function called to publish the temperature to homeassistant
 * 
 * @param json_val 
 */
void publishData(float temp, float humidity, float heatindex)
{
  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  //JsonArray arr = jb.createArray();
  DynamicJsonDocument doc(capacity);
  doc["OfficeTemperature"] = (String)temp;
  doc["Officehumidity"] = (String)humidity;
  doc["OfficeheatIndex"] = (String)heatindex;
  serializeJsonPretty(doc, Serial);
  Serial.println("");
  char output[256];
  // Produce a minified JSON document
  serializeJson(doc, output, measureJson(doc) + 1);
  client.publish(MQTT_SENSOR_TOPIC, output, true);
  yield();
}

/**
 * @brief Main loop of the program
 * 
 */
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
     client.connect("ESPofficeClient");

  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  else{
    // Computes temperature values in Celsius
    float hic = dht.computeHeatIndex(t, h, false);
    static char temperatureTemp[7];
    dtostrf(hic, 6, 2, temperatureTemp);
    
    static char humidityTemp[7];
    dtostrf(h, 6, 2, humidityTemp);

    // Publishes Temperature and Humidity values
    // Json
    //------------
    publishData(t, h, hic);
  }
  //Go to sleep now
	esp_deep_sleep_start();
}