#include "LoRaInterface.h"

#include <Log.h>
#include <Utilities/OS.h>

#include <memory>

using namespace RNS;

/*
 * arduinoLoRa Library just only support SX1276/Sx1278, not SX1262
 * RadioLib Library also supports SX1262/SX1268 see https://github.com/Xinyuan-LilyGO/LilyGo-LoRa-Series/tree/master/examples/RadioLibExamples
 * */

/*
@staticmethod
def get_address_for_if(name):
	import RNS.vendor.ifaddr.niwrapper as netinfo
	ifaddr = netinfo.ifaddresses(name)
	return ifaddr[netinfo.AF_INET][0]["addr"]

@staticmethod
def get_broadcast_for_if(name):
	import RNS.vendor.ifaddr.niwrapper as netinfo
	ifaddr = netinfo.ifaddresses(name)
	return ifaddr[netinfo.AF_INET][0]["broadcast"]
*/

LoRaInterface::LoRaInterface(const char* name /*= "LoRaInterface"*/) : Interface(name) {

	IN(true);
	OUT(true);

	//p self.bitrate = self.r_sf * ( (4.0/self.r_cr) / (math.pow(2,self.r_sf)/(self.r_bandwidth/1000)) ) * 1000
	bitrate((double)_spreading_factor * ( (4.0/_coding_rate) / (pow(2, _spreading_factor)/(_signal_bandwidth/1000.0)) ) * 1000.0);

}

LoRaInterface::LoRaInterface(long radio_frequency, long signal_bandwidth /*= DEFAULT_SIGNAL_BANDWIDTH*/, int spreading_factor /*= DEFAULT_SPREADING_FACTOR*/, int coding_rate /*= DEFAULT_CODING_RATE*/, int tx_power /*= DEFAULT_TX_POWER*/, const char* name /*= "LoRaInterface"*/) :
	Interface(name),
	_radio_frequency(radio_frequency),
	_signal_bandwidth(signal_bandwidth),
	_spreading_factor(spreading_factor),
	_coding_rate(coding_rate),
	_tx_power(tx_power)
{

	IN(true);
	OUT(true);

	//p self.bitrate = self.r_sf * ( (4.0/self.r_cr) / (math.pow(2,self.r_sf)/(self.r_bandwidth/1000)) ) * 1000
	bitrate((double)_spreading_factor * ( (4.0/_coding_rate) / (pow(2, _spreading_factor)/(_signal_bandwidth/1000.0)) ) * 1000.0);

}

/*virtual*/ LoRaInterface::~LoRaInterface() {
	stop();
}

bool LoRaInterface::start() {
	online(false);
	info("LoRa initializing...");
  
	// CBA Following is called from initBoard() in setup()
	//SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
	//delay(1500);

    LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);

	// initialize radio
	if (!LoRa.begin(_radio_frequency)) {
		error("LoRa init failed. Check your connections.");
		return false;
	}

	LoRa.setSignalBandwidth(_signal_bandwidth);
	LoRa.setSpreadingFactor(_spreading_factor);
	LoRa.setCodingRate4(_coding_rate);
	LoRa.setPreambleLength(20);
	LoRa.setTxPower(_tx_power);

	info("LoRa init succeeded.");
	extreme("LoRa bandwidth is " + std::to_string(Utilities::OS::round(bitrate()/1000.0, 2)) + " Kbps");

	online(true);
	return true;
}

void LoRaInterface::stop() {
	online(false);
}

void LoRaInterface::loop() {

	if (online()) {
		// Check for incoming packet
		int available = LoRa.parsePacket();
		if (available > 0) {
			extreme("LoRaInterface: receiving bytes...");

			// read header (for detecting split packets)
		    uint8_t header  = LoRa.read();

			// CBA TODO add support for split packets

			// read packet
			buffer.clear();
			while (LoRa.available()) {
				buffer << (uint8_t)LoRa.read();
			}

			on_incoming(buffer);

			Serial.println("RSSI: " + String(LoRa.packetRssi()));
			Serial.println("Snr: " + String(LoRa.packetSnr()));

#ifdef HAS_DISPLAY
			if (u8g2) {
				u8g2->clearBuffer();
				//char buf[256];
				String line;
				line = "Received " + String(buffer.size()) + " bytes";
				u8g2->drawStr(0, 12, line.c_str());
				//u8g2->drawStr(5, 26, str.c_str());
				//snprintf(buf, sizeof(buf), "RSSI: %.0f", LoRa.packetRssi());
				line = "RSSI: " + String(LoRa.packetRssi());
				u8g2->drawStr(0, 40, line.c_str());
				//snprintf(buf, sizeof(buf), "SNR: %.2f", LoRa.packetSnr());
				line = "SNR: " + String(LoRa.packetSnr());
				u8g2->drawStr(0, 54, line.c_str());
				u8g2->sendBuffer();
			}
#endif
		}
	}
}

/*virtual*/ void LoRaInterface::on_incoming(const Bytes& data) {
	debug("LoRaInterface.on_incoming: data: " + data.toHex());
	Interface::on_incoming(data);
}

/*virtual*/ void LoRaInterface::on_outgoing(const Bytes& data) {
	debug("LoRaInterface.on_outgoing: data: " + data.toHex());
	try {
		if (online()) {
			extreme("LoRaInterface: sending " + std::to_string(data.size()) + " bytes...");
			// Send packet

			LoRa.beginPacket();                   // start packet

			// write header (for detecting split packets)
		    uint8_t header  = Cryptography::randomnum(256) & 0xF0;
			LoRa.write(header);

			// CBA TODO add support for split packets

			// add payload
			//LoRa.print((const char*)data.data());
			for (size_t i = 0; i < data.size(); ++i) {
				LoRa.write(data.data()[i]);
			}

			LoRa.endPacket();                     // finish packet and send it

			extreme("LoRaInterface: sent bytes");

#ifdef HAS_DISPLAY
			if (u8g2) {
				u8g2->clearBuffer();
				String line;
				line = "Sent " + String(data.size()) + " bytes";
				u8g2->drawStr(0, 12, line.c_str());
				//u8g2->drawStr(5, 26, str.c_str());
				u8g2->sendBuffer();
			}
#endif
		}
		Interface::on_outgoing(data);
	}
	catch (std::exception& e) {
		error("Could not transmit on " + toString() + ". The contained exception was: " + e.what());
	}
}
