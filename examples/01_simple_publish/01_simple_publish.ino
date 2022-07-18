#include <Arduino.h>
#include <ArduinoJSON.h>
#include "ESPWiFiMqttWrapper.h"

//Code ini support untuk ESP32 dan ESP8266
//Membutuhkan library ArduinoJSON dan library PubSubClient
//https://www.arduino.cc/reference/en/libraries/arduinojson/
//https://www.arduino.cc/reference/en/libraries/pubsubclient/
ESPWiFiMqttWrapper wrapper;


const char* MQTT_Server		= "iot.a2n.tech"; // server MQTT
const char* MQTT_username	= "Your User Name"; // login user anda pada aplikasi
const char* MQTT_password	= "Your Password"; // password login anda pada aplikasi

const char* WiFi_SSID		= "Your WiFi SSID"; // nama wifi anda
const char* WiFi_Password	= "Your WiFi Password"; // password wifi anda
const char* WiFi_HostName	= "yourHostName"; // nama host name device ini

//	Anda hanya diperbolehkan publish atau subscribe dengan awalan Topic yang telah ditentukan pada aplikasi
//	Root topic dapat ditemukan pada halaman utama (Main Dashboard) http://iot.a2n.tech
//	Sub Topic bebas, namun format sub topic harus sesuai dengan ketentuan standar MQTT
//	contoh format topic:
//	{ROOT_TOPIC}/{SUB_TOPIC}
//	jika ROOT_TOPIC anda adalah /user/12345/ dan sub topic yg anda inginkan adalah esp32/suhu
//	maka Topicnya adalah /user/12345/esp32/suhu
//  const char* MQTT_PUBLISH_TOPIC = "/user/12345/esp32/suhu";
const char* MQTT_PUBLISH_TOPIC = "/{ROOT_TOPIC}/{SUB_TOPIC}";

// publish interval dalam satuan milisekon
uint32_t PublishInterval = 10000; // setiap 10 detik

void setup() {
	Serial.begin(115200);

	// enable debugger to Serial
	wrapper.setDebugger(&Serial);

	wrapper.setWiFi(WiFi_HostName, WiFi_SSID, WiFi_Password);
	wrapper.setMqttServer(MQTT_Server, MQTT_username, MQTT_password);
	wrapper.initWiFi();

	//param1: topic
	//param2: interval
	//param3: fungsi membuat message
	wrapper.setPublisher(MQTT_PUBLISH_TOPIC, PublishInterval, [&] {
		String message = "";
		DynamicJsonDocument doc(512);

		float temp = random(2500.0f, 3500.0f) / 100.0f;
		float hum = random(7500.0f, 9000.0f) / 100.0f;

		// contoh membuat dokumen JSON
		doc["temperature"] = temp;
		doc["humidity"] = hum;

		// Serialisasi object doc menjadi format JSON ke variabel message
		// Contoh Output messge => { "temperature": 25,"humidity":75 }
		serializeJson(doc, message);
		return message;
	});

	wrapper.initMqtt();
}
void loop() {
	wrapper.loop();
}