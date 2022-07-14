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


#ifndef ESPWiFiMqttWrapper_H
#define ESPWiFiMqttWrapper_H

//PubSubClient (require install)
//https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <PubSubClient.h>

//Arduino JSON Library (require install)
//https://www.arduino.cc/reference/en/libraries/arduinojson/
#include <ArduinoJson.h>

// ESP32 Built In library
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

typedef std::function<void(char*, uint8_t*, unsigned int)> ArSubscribeHandlerFunction;
typedef std::function<void(const char*)> ArSubscribeMessageHandlerFunction;
typedef std::function<String()> ArPublishHandlerFunction;

template <typename T>
class ListOfNode {
	T _value;
public:
	ListOfNode<T>* next;
	ListOfNode(const T val) : _value(val), next(nullptr) {}
	~ListOfNode() {}
	const T& value() const { return _value; };
	T& value() { return _value; }
};
template <typename T, template<typename> class Item = ListOfNode>
class ListOf {
public:
	typedef Item<T> ItemType;
	typedef std::function<void(const T&)> OnRemove;
	typedef std::function<bool(const T&)> Predicate;
private:
	ItemType* _root;
	OnRemove _onRemove;

	class Iterator {
		ItemType* _node;
	public:
		Iterator(ItemType* current = nullptr) : _node(current) {}
		Iterator(const Iterator& i) : _node(i._node) {}
		Iterator& operator ++() { _node = _node->next; return *this; }
		bool operator != (const Iterator& i) const { return _node != i._node; }
		const T& operator * () const { return _node->value(); }
		const T* operator -> () const { return &_node->value(); }
	};

public:
	typedef const Iterator ConstIterator;
	ConstIterator begin() const { return ConstIterator(_root); }
	ConstIterator end() const { return ConstIterator(nullptr); }

	ListOf(OnRemove onRemove) : _root(nullptr), _onRemove(onRemove) {}
	~ListOf() {}
	void add(const T& t) {
		auto it = new ItemType(t);
		if (!_root) {
			_root = it;
		}
		else {
			auto i = _root;
			while (i->next) i = i->next;
			i->next = it;
		}
	}
	T& front() const {
		return _root->value();
	}

	bool isEmpty() const {
		return _root == nullptr;
	}
	size_t length() const {
		size_t i = 0;
		auto it = _root;
		while (it) {
			i++;
			it = it->next;
		}
		return i;
	}
	size_t count_if(Predicate predicate) const {
		size_t i = 0;
		auto it = _root;
		while (it) {
			if (!predicate) {
				i++;
			}
			else if (predicate(it->value())) {
				i++;
			}
			it = it->next;
		}
		return i;
	}
	const T* nth(size_t N) const {
		size_t i = 0;
		auto it = _root;
		while (it) {
			if (i++ == N)
				return &(it->value());
			it = it->next;
		}
		return nullptr;
	}
	bool remove(const T& t) {
		auto it = _root;
		auto pit = _root;
		while (it) {
			if (it->value() == t) {
				if (it == _root) {
					_root = _root->next;
				}
				else {
					pit->next = it->next;
				}

				if (_onRemove) {
					_onRemove(it->value());
				}

				delete it;
				return true;
			}
			pit = it;
			it = it->next;
		}
		return false;
	}
	bool remove_first(Predicate predicate) {
		auto it = _root;
		auto pit = _root;
		while (it) {
			if (predicate(it->value())) {
				if (it == _root) {
					_root = _root->next;
				}
				else {
					pit->next = it->next;
				}
				if (_onRemove) {
					_onRemove(it->value());
				}
				delete it;
				return true;
			}
			pit = it;
			it = it->next;
		}
		return false;
	}

	void free() {
		while (_root != nullptr) {
			auto it = _root;
			_root = _root->next;
			if (_onRemove) {
				_onRemove(it->value());
			}
			delete it;
		}
		_root = nullptr;
	}
};

class SubscribeHandler {
protected:
	const char* _topicFilter;
	ArSubscribeMessageHandlerFunction _func1;
	ArSubscribeHandlerFunction _func2;
public:
	const char* getTopicFilter() {
		return _topicFilter;
	}
	void setTopicFilter(const char* topicFilter) { _topicFilter = topicFilter; }
	void setFunction(ArSubscribeMessageHandlerFunction func) { _func1 = func; }
	void setFunction(ArSubscribeHandlerFunction func) { _func2 = func; }
	bool canHandle(const char* topic) {
		return strcmp(topic, _topicFilter) == 0;
	}
	void handleFunction(char* topic, uint8_t* payload, unsigned int length) {
		if (_func1) {
			String message;
			for (int i = 0; i < length; i++)
				message += (char)payload[i];
			_func1(message.c_str());
		}
		else if (_func2)
			_func2(topic, payload, length);
	}
};

class PublishHandler {
protected:
	const char* _topic;
	long _startDelay = 0;
	long _interval = 1000;
	long _delta = 0;
	uint32_t _lastMillis = 0;
	ArPublishHandlerFunction _func;
public:
	void setTopic(const char* topic) { _topic = topic; }
	const char* getTopic() {
		return _topic;
	}
	void setFunction(ArPublishHandlerFunction func) { _func = func; }
	void setInterval(long interval) { _interval = interval; }
	void setStartDelay(long startDelay) { _startDelay = startDelay; }
	bool canHandle(uint32_t now) {
		if (_startDelay > now)
			return false;
		if (_lastMillis > now)
			_lastMillis = 0;
		_delta = now - _lastMillis;
		return _interval <= _delta;
	}
	const char * handleFunction(void) {
		const char * result = nullptr;
		if (_func) {
			String val = _func();
			result = val.c_str();
		}
		
		_lastMillis = millis();
		return result;
	}
};

class ESPWiFiMqttWrapper {
private:
	const char* _mqttServer = "iot.a2n.tech";
	uint16_t _mqttPort = 1883;

	PubSubClient _mqttClient;
	ListOf<SubscribeHandler*> _subscribehandlers;
	ListOf<PublishHandler*> _publishHandlers;
	Stream* _debugger;

	int _maxReconnect = 30;
	bool _debug;
	const char* _mqttUsername;
	const char* _mqttPassword;
	const char* _mqttClientId;
	const char* _wifiHostName;
	const char* _wifiSSID;
	const char* _wifiPass;

	bool connectMqtt();
	bool connectWiFi();

	SubscribeHandler& addSubscribeHandler(SubscribeHandler* handler) {
		_subscribehandlers.add(handler);
		return *handler;
	};
	bool removeSubscribeHandler(SubscribeHandler* handler) {
		return _subscribehandlers.remove(handler);
	};
	PublishHandler& addPublishHandler(PublishHandler* handler) {
		_publishHandlers.add(handler);
		return *handler;
	};
	bool removePublishHandler(PublishHandler* handler) {
		return _publishHandlers.remove(handler);
	};
	void print(const String& s) {
		if (!_debug)
			return;
		_debugger->print(s);
	};
	void print(const char* c) {
		if (!_debug)
			return;
		_debugger->print(c);
	};
	void print(char c) {
		if (!_debug)
			return;
		_debugger->print(c);
	};
	void println(const String& s) {
		if (!_debug)
			return;
		_debugger->println(s);
	};
	void println(char c) {
		if (!_debug)
			return;
		_debugger->println(c);
	};
	void println(const char* c) {
		if (!_debug)
			return;
		_debugger->println(c);
	};
	void println(const Printable& p) {
		if (!_debug)
			return;
		_debugger->println(p);
	};
	void println(void) {
		if (!_debug)
			return;
		_debugger->println();
	};
	void setMqttServer();
public:
	ESPWiFiMqttWrapper();
	void setMqttServer(const char* UserName, const char* Password);
	void setMqttServer(const char* mqttServer, const char* UserName, const char* Password);
	void setMqttServer(const char* mqttServer, uint16_t mqttPort, const char* UserName, const char* Password);
	void setDebugger(Stream* debugger) {
		_debug = true;
		_debugger = debugger;
	}
	SubscribeHandler& setSubscription(const char* topicFilter, ArSubscribeHandlerFunction func);
	SubscribeHandler& setSubscription(const char* topicFilter, ArSubscribeMessageHandlerFunction func);
	PublishHandler& setPublisher(const char* topic, int interval, ArPublishHandlerFunction func);
	PublishHandler& setPublisher(const char* topic, int interval, int startDelay, ArPublishHandlerFunction func);
	void setMaxReconnect(int value) {
		_maxReconnect = value;
	};
	void setWiFi(const char* hostName, const char* SSID, const char* wifiPassword);
	void setWiFi(WiFiClient client);
	void initWiFi();
	void initMqtt();
	bool loop();
	PubSubClient getMqttClient() {
		return _mqttClient;
	};
	///* Overloaded functions end */
	//void printData();
	bool publish(const char* topic, const char* payload) {
		return _mqttClient.publish(topic, payload);
	}
	bool publish(const char* topic, const char* payload, boolean retained) {
		return _mqttClient.publish(topic, payload, retained);
	}
	bool publish(const char* topic, const uint8_t* payload, unsigned int plength) {
		return _mqttClient.publish(topic, payload, plength);
	}
	bool publish(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained) {
		return _mqttClient.publish(topic, payload, plength, retained);
	}
	bool publish_P(const char* topic, const char* payload, boolean retained) {
		return _mqttClient.publish_P(topic, payload, retained);
	}
	bool publish_P(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained) {
		return _mqttClient.publish_P(topic, payload, plength, retained);
	}
};
#endif
