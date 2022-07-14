#include <Arduino.h>
#include <ArduinoJSON.h>
#include "ESPWiFiMqttWrapper.h"

//Code ini support untuk ESP32 dan ESP8266
//Membutuhkan library ArduinoJSON dan library PubSubClient
//https://www.arduino.cc/reference/en/libraries/arduinojson/
//https://www.arduino.cc/reference/en/libraries/pubsubclient/
ESPWiFiMqttWrapper wrapper;


const char* MQTT_Server = "iot.a2n.tech"; // server MQTT
const char* MQTT_username = "Your User Name"; // login user anda pada aplikasi
const char* MQTT_password = "Your Password"; // password login anda pada aplikasi

const char* WiFi_SSID = "Your WiFi SSID"; // nama wifi anda
const char* WiFi_Password = "Your WiFi Password"; // password wifi anda
const char* WiFi_HostName = "yourHostName"; // nama host name device ini

//	Anda hanya diperbolehkan publish atau subscribe dengan awalan Topic yang telah ditentukan pada aplikasi
//	Root topic dapat ditemukan pada halaman utama (Main Dashboard) http://iot.a2n.tech
//	Sub Topic bebas, namun format sub topic harus sesuai dengan ketentuan standar MQTT
//	contoh format topic:
//	{ROOT_TOPIC}/{SUB_TOPIC}
//	jika ROOT_TOPIC anda adalah /user/12345/ dan sub topic yg anda inginkan adalah esp32/suhu
//	maka Topicnya adalah /user/12345/esp32/suhu
//  const char* MQTT_PUBLISH_TOPIC = "/user/12345/esp32/suhu";
const char* MQTT_SUBSCRIBE_TOPIC_TEMP = "{ROOT_TOPIC}/esp/temp";
const char* MQTT_SUBSCRIBE_TOPIC_COMMAND = "{ROOT_TOPIC}/esp/command";

#if defined(ESP8266)
uint8_t PIN_DIGITAL_OUTPUT[] = { 4, 5, 12, 13, 15 };
uint8_t PIN_ANALOG_OUTPUT[] = { 2 };
#elif defined(ESP32)
uint8_t PIN_DIGITAL_OUTPUT[] = { 4, 5, 13, 15, 16, 17, 18, 19 };
uint8_t PIN_ANALOG_OUTPUT[] = { 25, 26 };
#endif

void setupPinOutput() {
	int len = 0;
	int i = 0;
	len = sizeof(PIN_DIGITAL_OUTPUT);
	for (i = 0; i < len; i++)
		pinMode(PIN_DIGITAL_OUTPUT[i], OUTPUT);

	len = sizeof(PIN_ANALOG_OUTPUT);
	for (i = 0; i < len; i++)
		pinMode(PIN_ANALOG_OUTPUT[i], OUTPUT);
}
bool checkAvailablePin(uint8_t pins[], uint8_t pin) {
	int len = sizeof(pins);
	for (int i = 0; i < len; i++)
	{
		if (pins[i] == pin)
			return true;
	}
	return false;
}

void setup() {
	Serial.begin(115200);

	setupPinOutput();

	// enable debugger to Serial
	wrapper.setDebugger(&Serial);

	wrapper.setWiFi(WiFi_HostName, WiFi_SSID, WiFi_Password);
	wrapper.setMqttServer(MQTT_Server, MQTT_username, MQTT_password);
	wrapper.initWiFi();

	wrapper.setSubscription(MQTT_SUBSCRIBE_TOPIC_TEMP, [&](const char* message) {
		Serial.print("Message Received : ");
		Serial.print(message);
		Serial.println();

		////jika message adalah format json
		////contoh jika message-nya adalah { "temp": 28.3, "hum": 78.4 }
		//DynamicJsonDocument doc(512);
		//deserializeJson(doc, message);
		//float temperature = doc["temp"];
		//float humidity = doc["hum"];
		//Serial.print("Temperature: ");
		//Serial.print(temperature);
		//Serial.print(", Humidity: ");
		//Serial.println(humidity);
	});

	wrapper.setSubscription(MQTT_SUBSCRIBE_TOPIC_COMMAND, [&](const char* message) {
		Serial.print("Message Received : ");
		Serial.print(message);
		Serial.println();

		// format json { "gpio": 4, "value": 1 }
		DynamicJsonDocument doc(512);
		deserializeJson(doc, message);
		// validasi data
		if (doc["gpio"].is<uint8_t>() && doc["value"].is<uint8_t>()) {
			uint8_t gpio = doc["gpio"];
			uint8_t value = doc["value"];
			bool isAnalog = false;
			bool isValid = false;

			isValid = checkAvailablePin(PIN_DIGITAL_OUTPUT, gpio);
			if (!isValid) {
				isValid = checkAvailablePin(PIN_ANALOG_OUTPUT, gpio);
				isAnalog = true;
			}
			if (isValid) {
				if (!isAnalog) {
					if (value)
						digitalWrite(gpio, HIGH);
					else
						digitalWrite(gpio, LOW);
				}
				else {
#if defined(ESP8266)
					analogWrite(gpio, value);
#elif defined(ESP32)
					dacWrite(gpio, value);
#endif
				}
			}
			else {
				Serial.print("GPIO :");
				Serial.print(gpio);
				Serial.println(" tidak tersedia");
			}
		}
		else
			Serial.println("Format data salah !!.. contoh format format json { \"gpio\": 4, \"value\": 1 }");
	});

	wrapper.initMqtt();
}
void loop() {
	wrapper.loop();
}