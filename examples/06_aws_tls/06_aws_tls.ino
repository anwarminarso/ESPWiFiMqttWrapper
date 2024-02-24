
#include <Arduino.h>
#include <ESPWiFiMqttWrapper.h>
#include <ArduinoJson.h>
#include "certificate.h"

ESPWiFiMqttWrapper wrapper;

// AWS Endpoint URL (check on AWS IoT Console -> Settings -> Device Endpoint)
const char* MQTT_Server		= "xxxxxxxx.iot.us-east-1.amazonaws.com";
uint16_t	MQTT_PORT		= 8883;
const char* MQTT_CLIENT_ID	= "Your Thing Name"; // nama thing di AWS IoT

const char* WiFi_SSID		= "Your WiFi SSID"; // nama wifi anda
const char* WiFi_Password	= "Your WiFi Password"; // password wifi anda
const char* WiFi_HostName	= "yourHostName"; // nama host name device ini

void setup() {
	Serial.begin(115200);
	wrapper.setDebugger(&Serial);

	wrapper.setWiFi(WiFi_HostName, WiFi_SSID, WiFi_Password);
	wrapper.setCACert(CA_CERT);
	wrapper.setCertificate(DEVICE_CERT);
	wrapper.setPrivateKey(DEVICE_KEY);
	wrapper.setMqttClientId(MQTT_CLIENT_ID);
	wrapper.setMqttServer(MQTT_Server, MQTT_PORT);

	wrapper.initWiFi();

	// Subscribing to and receiving messages to a topic must be allowed by an AWS IoT Policy
	wrapper.setSubscription("esp32/sub", [&](const char* message) {
		Serial.print("Message Received : ");
		Serial.print(message);
		Serial.println();
	});

	// Publishing to a topic must be allowed by an AWS IoT Policy
	wrapper.setPublisher("esp32/pub", 3000, [&] {
		String message = "";
		JsonDocument doc;

		float temp = random(2500.0f, 3500.0f) / 100.0f;
		float hum = random(7500.0f, 9000.0f) / 100.0f;

		// create JSON document
		doc["temperature"] = temp;
		doc["humidity"] = hum;

		// Serialize to JSON
		// Excample Output messge => { "temperature": 25,"humidity":75 }
		serializeJson(doc, message);
		return message;
	});
	wrapper.initMqtt();

}
void loop()
{
	wrapper.loop();
}
