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
const char* MQTT_Publish_TOPIC_TEMP = "{ROOT_TOPIC}/esp/temp";
const char* MQTT_Publish_TOPIC_GPS =  "{ROOT_TOPIC}/esp/gps";
const char* MQTT_Publish_TOPIC_NOTIFIKASI =  "{ROOT_TOPIC}/esp/alert";

uint32_t PublishInterval_TEMP = 10000;	//publish temperature dan humidity setiap 10 detik
uint32_t PublishInterval_GPS = 30000;	//publish GPS setiap 30 detik

static float temperature;
static float humidity;

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
	wrapper.setPublisher(MQTT_Publish_TOPIC_TEMP, PublishInterval_TEMP, [&] {
		String message = "";
		DynamicJsonDocument doc(512);

		temperature = random(2500f, 3500f) / 100.0f;
		humidity = random(7500f, 9000f) / 100.0f;
		
		// contoh membuat dokumen JSON
		doc["temperature"] = temperature;
		doc["humidity"] = humidity;

		// Serialisasi object doc menjadi format JSON ke variabel message
		// Contoh Output messge => { "temperature": 25,"humidity":75 }
		serializeJson(doc, message);
		return message;
	});


	//param1: topic
	//param2: interval
	//param3: fungsi membuat message
	wrapper.setPublisher(MQTT_Publish_TOPIC_GPS, PublishInterval_GPS, [&] {
		String message = "";
		DynamicJsonDocument doc(512);
		
		float lat = -6.3023799f;
		float lon = 106.7189562f;

		// contoh membuat dokumen JSON
		doc["latitude"] = lat;
		doc["longitude"] = lon;

		// Serialisasi object doc menjadi format JSON ke variabel message
		// Contoh Output messge => { "latitude": -6.3023799,"longitude":106.7189562 }
		serializeJson(doc, message);
		return message;
	});
	

	wrapper.initMqtt();
}
void loop() {
	wrapper.loop();
	
	if (temperature > 30) {
		// publish jika ada kejadian suhu lebih dari 30 derajat
		wrapper.publish(MQTT_Publish_TOPIC_NOTIFIKASI, "Suhu lebih dari 30");
		delay(1000);
	}
}