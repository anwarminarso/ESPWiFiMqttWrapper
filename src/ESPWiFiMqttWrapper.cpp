 //------------------------------------------------------------------
 // Copyright(c) 2022 a2n Technology
 // Anwar Minarso (anwar.minarso@gmail.com)
 // https://github.com/anwarminarso/
 // This file is part of the a2n ESPWiFiMqttWrapper
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
WiFiClient _client;
int _reconnectMqttCount = 0;
int _reconnectWifiCount = 0;
int _lastReconnect = 0;
unsigned long now;
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
void ESPWiFiMqttWrapper::setWiFi(WiFiClient client) {
	_client = client;
}
void ESPWiFiMqttWrapper::initWiFi() {
	int connectionAttempt = 0;
#if defined(ESP8266)
	WiFi.disconnect(true);
	delay(1000);
#elif defined(ESP32)
	WiFi.persistent(false);
	WiFi.disconnect(true, true);
	delay(100);
	WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
	delay(500);
#endif
	if (!WiFi.setHostname(this->_wifiHostName))
	{
		this->print("Failure to set hostname. Current Hostname : ");
		this->println(WiFi.getHostname());
	}
	WiFi.mode(WIFI_STA);
	WiFi.begin(this->_wifiSSID, this->_wifiPass);
	this->print("Connecting to WiFi ..");
	while (WiFi.status() != WL_CONNECTED) {
		this->print('.');
		connectionAttempt++;
		if (connectionAttempt >= (_maxReconnect + 1)) {
			ESP.restart();
		}
		delay(1000);
	}
	this->println(WiFi.localIP());
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
	_mqttClient.setClient(_client);
#if defined(ESP8266)
	String clientId = "ESP8266-";
#elif defined(ESP32)
	String clientId = "ESP32-";
#endif
	
	clientId += getWifiMacAddress();
	_mqttClientId = clientId.c_str();
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
		
		this->print("Attempting MQTT connection: ");
		// Attempt to connect
		if (_mqttClient.connect(_mqttClientId, _mqttUsername, _mqttPassword)) {
			this->println("connected");
			for (const auto& h : _subscribehandlers) {
				_mqttClient.subscribe(h->getTopicFilter());
			}
			_reconnectMqttCount = 0;
			result = true;
		}
		else {
			this->print("Failed, Reason Code=");
			this->print(_mqttClient.state());
			this->println();
		}
	}
	else
		result = true;
	return result;
}
bool ESPWiFiMqttWrapper::connectWiFi() {
	bool result = false;
	if (WiFi.status() == WL_CONNECTED) {
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
			this->println("Attempting WiFi connection...");
			WiFi.reconnect();
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