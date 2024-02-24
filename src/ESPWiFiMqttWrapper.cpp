//------------------------------------------------------------------
// Copyright(c) 2022-2024 a2n Technology
// Anwar Minarso (anwar.minarso@gmail.com)
// https://github.com/anwarminarso/
// This file is part of the a2n ESPWiFiMqttWrapper v1.0.6
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
// Lesser General Public License for more details
//------------------------------------------------------------------

#include "ESPWiFiMqttWrapper.h"
String getWifiMacAddress() {
	String macAddress = WiFi.macAddress();
	macAddress.remove(14, 1);
	macAddress.remove(11, 1);
	macAddress.remove(8, 1);
	macAddress.remove(5, 1);
	macAddress.remove(2, 1);
	macAddress.toUpperCase();
	return macAddress;
}

ESPWiFiMqttWrapper::ESPWiFiMqttWrapper() :
	_subscribehandlers(ListOf<SubscribeHandler*>([](SubscribeHandler* h) { delete h; })),
	_publishHandlers(ListOf<PublishHandler*>([](PublishHandler* h) { delete h; }))
{
}

void ESPWiFiMqttWrapper::setWiFi(const char* HostName, const char* SSID, const char* Password) {
	this->_wifiHostName = HostName;
	this->_wifiSSID = SSID;
	this->_wifiPass = Password;
}
void ESPWiFiMqttWrapper::setMqttClientId(const char* ClientId) {
	this->_mqttClientId = ClientId;
}

#if defined(ESP32)
void ESPWiFiMqttWrapper::setCACert(const char* certificate) {
	this->_secureClient.setCACert(certificate);
	this->_useSecureWiFi = true;
}
void ESPWiFiMqttWrapper::setCertificate(const char* certificate) {
	this->_secureClient.setCertificate(certificate);
	this->_useSecureWiFi = true;
}
void ESPWiFiMqttWrapper::setPrivateKey(const char* privateKey) {
	this->_secureClient.setPrivateKey(privateKey);
	this->_useSecureWiFi = true;
}
#elif defined (ESP8266)
void ESPWiFiMqttWrapper::setCACert(const X509List* certificate) {
	this->_secureClient.setTrustAnchors(certificate);
	this->_useSecureWiFi = true;
}
void ESPWiFiMqttWrapper::setCertificate(const X509List* certificate, const PrivateKey* privateKey) {
	this->_secureClient.setClientRSACert(certificate, privateKey);
	this->_useSecureWiFi = true;
}

#else
#error "Unsupported platform"
#endif

#if defined(ESP8266)
void ESPWiFiMqttWrapper::setClock() {
	configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
	this->print("Waiting for NTP time sync: ");
	time_t now = time(nullptr);
	while (now < 8 * 3600 * 2) {
		delay(500);
		this->print(".");
		now = time(nullptr);
	}
	this->println("");
	struct tm timeinfo;
	gmtime_r(&now, &timeinfo);
	this->print("Current time (UTC): ");
	this->print(asctime(&timeinfo));
}
#endif
void ESPWiFiMqttWrapper::initWiFi() {
	_reconnectWifiCount = 0;
#if defined(ESP8266)
	WiFi.disconnect(true);
	delay(1000);
#elif defined(ESP32)
	WiFi.persistent(false);
	WiFi.disconnect(true, true);
	delay(100);
	WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
	delay(500);
	if (!WiFi.setHostname(this->_wifiHostName))
	{
		this->print("Failure to set hostname. Current Hostname : ");
		this->println(WiFi.getHostname());
	}
#endif
	WiFi.mode(WIFI_STA);
	WiFi.begin(this->_wifiSSID, this->_wifiPass);
	this->print("Connecting to WiFi ..");
	while (WiFi.status() != WL_CONNECTED) {
		this->print('.');
		_reconnectWifiCount++;
		if (_reconnectWifiCount >= (_maxReconnect + 1)) {
			ESP.restart();
		}
		delay(1000);
	}
	this->print("Connected. IP Address: ");
	this->println(WiFi.localIP());
	_reconnectWifiCount = 0;
#if defined(ESP8266)
	if (_useSecureWiFi) {
		setClock();
	}
#endif
}

void ESPWiFiMqttWrapper::setMqttServer(const char* mqttServer) {
	this->_mqttServer = mqttServer;
	setMqttServer();
}

void ESPWiFiMqttWrapper::setMqttServer(const char* mqttServer, uint16_t mqttPort) {
	this->_mqttServer = mqttServer;
	this->_mqttPort = mqttPort;
	setMqttServer();
}
void ESPWiFiMqttWrapper::setMqttServer(const char* mqttServer, uint16_t mqttPort, const char* UserName, const char* Password) {
	this->_mqttServer = mqttServer;
	this->_mqttPort = mqttPort;
	this->_mqttUsername = UserName;
	this->_mqttPassword = Password;
	setMqttServer();
}
void ESPWiFiMqttWrapper::setMqttServer(const char* mqttServer, const char* UserName, const char* Password) {
	this->_mqttServer = mqttServer;
	this->_mqttUsername = UserName;
	this->_mqttPassword = Password;
	setMqttServer();
}
void ESPWiFiMqttWrapper::setMqttServer(const char* UserName, const char* Password) {
	this->_mqttUsername = UserName;
	this->_mqttPassword = Password;
	setMqttServer();
}
void ESPWiFiMqttWrapper::setMqttServer() {
	if (this->_useSecureWiFi) {
		_mqttClient.setClient(_secureClient);
	}
	else {
		_mqttClient.setClient(_defaultClient);
	}
	if (this->_mqttClientId && !this->_mqttClientId[0]) {
#if defined(ESP8266)
		String clientId = "ESP8266-";
#elif defined(ESP32)
		String clientId = "ESP32-";
#endif
		clientId += getWifiMacAddress();
		int str_len = clientId.length() + 1;
		char* clientIdChars = (char*)malloc(str_len);
		clientId.toCharArray(clientIdChars, str_len);
		_mqttClientId = clientIdChars;
		this->println(_mqttClientId);
	}
	_mqttClient.setServer(_mqttServer, _mqttPort);
}

SubscribeHandler& ESPWiFiMqttWrapper::setSubscription(const char* topicFilter, ArSubscribeHandlerFunction func) {
	SubscribeHandler* handler = new SubscribeHandler();
	handler->setTopicFilter(topicFilter);
	handler->setFunction(func);
	this->addSubscribeHandler(handler);
	return *handler;
}
SubscribeHandler& ESPWiFiMqttWrapper::setSubscription(const char* topicFilter, ArSubscribeMessageHandlerFunction func) {
	SubscribeHandler* handler = new SubscribeHandler();
	handler->setTopicFilter(topicFilter);
	handler->setFunction(func);
	this->addSubscribeHandler(handler);
	return *handler;
}
PublishHandler& ESPWiFiMqttWrapper::setPublisher(const char* topic, int interval, ArPublishHandlerFunction func) {
	PublishHandler* handler = new PublishHandler();
	handler->setTopic(topic);
	handler->setInterval(interval);
	handler->setFunction(func);
	this->addPublishHandler(handler);
	return *handler;
}
PublishHandler& ESPWiFiMqttWrapper::setPublisher(const char* topic, int interval, int startDelay, ArPublishHandlerFunction func) {
	PublishHandler* handler = new PublishHandler();
	handler->setTopic(topic);
	handler->setInterval(interval);
	handler->setStartDelay(startDelay);
	handler->setFunction(func);
	this->addPublishHandler(handler);
	return *handler;
}
void ESPWiFiMqttWrapper::removePublisher(const char* topic) {
	this->_publishHandlers.remove_first([topic](PublishHandler* h) {
		return h->isTopicEqual(topic);
	});
}
void ESPWiFiMqttWrapper::removeSubscription(const char* topicFilter) {
	this->_subscribehandlers.remove_first([topicFilter](SubscribeHandler* h) {
		return h->canHandle(topicFilter);
	});
}
void ESPWiFiMqttWrapper::initMqtt() {
	_mqttClient.setCallback([&](char* topic, uint8_t* payload, unsigned int length) {
		for (const auto& h : _subscribehandlers) {
			if (h->canHandle(topic)) {
				h->handleFunction(topic, payload, length);
			}
		}
	});
}
bool ESPWiFiMqttWrapper::connectMqtt() {
	bool result = false;
	if (!_mqttClient.connected()) {
		if (millis() - _lastReconnect < 1000)
			return result;
		_reconnectMqttCount++;
		_lastReconnect = millis();
		if (_reconnectMqttCount > _maxReconnect) {
			this->println("Restart ESP...");
			ESP.restart();
		}
		if (_useSecureWiFi) {
			this->print("Attempting MQTT secure connection: ");
		}
		else {
			this->print("Attempting MQTT connection: ");
		}
		// Attempt to connect
		if (_mqttClient.connect(_mqttClientId, _mqttUsername, _mqttPassword)) {
			this->print("Connected, MQTT Client Id: ");
			this->println(_mqttClientId);

			for (const auto& h : _subscribehandlers) {
				this->print("Subscribing to ");
				this->println(h->getTopicFilter());
				_mqttClient.subscribe(h->getTopicFilter());
			}
			_reconnectMqttCount = 0;
			result = true;
		}
		else {
			this->print("Failed, Reason Code=");
			this->print(String(_mqttClient.state()));
			this->println();
#if defined(ESP32) || defined(ESP8266)
			if (_useSecureWiFi) {
				char buf[80];
#if defined(ESP32)
				int error = _secureClient.lastError(buf, sizeof(buf));
				if (error) {
					this->println("SSL Error: " + String(error) + ", " + buf);
				}
#elif defined(ESP8266)
				int sslError = _secureClient.getLastSSLError(buf, sizeof(buf));
				if (sslError) {
					this->println("SSL Error: " + String(sslError) + ", " + buf);
				}
#endif
#endif
			}
		}
	}
	else
		result = true;
	return result;
}
bool ESPWiFiMqttWrapper::connectWiFi() {
	bool result = false;
	if (WiFi.status() == WL_CONNECTED) {
		if (_reconnectWifiCount > 0) {
			this->print("Connected. IP Address: ");
			this->println(WiFi.localIP());
		}
		_reconnectWifiCount = 0;
		result = true;
	}
	else {
		if (millis() - _lastReconnect < 1000)
			return result;
		_reconnectWifiCount++;
		_lastReconnect = millis();
		if (_reconnectWifiCount > _maxReconnect) {
			this->println("Restart ESP...");
			ESP.restart();
		}
		else {
			if (_reconnectWifiCount == 1) {
				this->print("Attempting WiFi connection...");
				WiFi.reconnect();
			}
			else {
				this->print(".");
			}
		}
	}
	return result;
}
bool ESPWiFiMqttWrapper::loop() {
	if (!connectWiFi())
		return false;
	if (!connectMqtt())
		return false;
	_mqttClient.loop();
	for (const auto& h : _publishHandlers) {
		now = millis();
		if (h->canHandle(now)) {
			const char* message = h->handleFunction();
			if (message != nullptr) {
				_mqttClient.publish(h->getTopic(), message);
			}
		}
	}
	return true;
}