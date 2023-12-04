#pragma once

#include <Interface.h>
#include <Bytes.h>
#include <Type.h>

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include "variants/config.h"
#include "variants/init.h"

#include <stdint.h>

#define DEFAULT_RADIO_FREQUENCY		915E6
#define DEFAULT_SIGNAL_BANDWIDTH	125E3
#define DEFAULT_SPREADING_FACTOR	8
#define DEFAULT_CODING_RATE			5
#define DEFAULT_TX_POWER			7

class LoRaInterface : public RNS::Interface {

public:
	LoRaInterface(const char* name = "LoRaInterface");
	LoRaInterface(long radio_frequency, long signal_bandwidth = DEFAULT_SIGNAL_BANDWIDTH, int spreading_factor = DEFAULT_SPREADING_FACTOR, int coding_rate = DEFAULT_CODING_RATE, int tx_power = DEFAULT_TX_POWER, const char* name = "LoRaInterface");
	virtual ~LoRaInterface();

	bool start();
	void stop();
	void loop();

	virtual inline std::string toString() const { return "LoRaInterface[" + name() + "]"; }

private:
	virtual void on_incoming(const RNS::Bytes& data);
	virtual void on_outgoing(const RNS::Bytes& data);

private:
	const uint16_t HW_MTU = 508;
	RNS::Bytes buffer;

	// Reticulum default
	long _radio_frequency = DEFAULT_RADIO_FREQUENCY;
	long _signal_bandwidth = DEFAULT_SIGNAL_BANDWIDTH;
	int _spreading_factor = DEFAULT_SPREADING_FACTOR;
	int _coding_rate = DEFAULT_CODING_RATE;
	int _tx_power = DEFAULT_TX_POWER;

};
