# DHT22-Hassio

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
