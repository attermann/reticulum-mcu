#include "RadioLibInterface.h"

#include <Log.h>
#include <Utilities/OS.h>

#include <memory>

using namespace RNS;

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

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
//bool transmitFlag = false;
enum Operation {
	OPERATION_NONE,
	OPERATION_RECEIVE,
	OPERATION_TRANSMIT
};
Operation operation = OPERATION_NONE;

// flag to indicate that a packet was sent or received
volatile bool operationDone = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
void setFlag(void) {
  // we sent or received a packet, set the flag
  operationDone = true;
}

RadioLibInterface::RadioLibInterface(const char* name /*= "RadioLibInterface"*/) : Interface(name) {

	IN(true);
	OUT(true);
	//p self.bitrate = self.r_sf * ( (4.0/self.r_cr) / (math.pow(2,self.r_sf)/(self.r_bandwidth/1000)) ) * 1000
	bitrate((double)spreading * ( (4.0/coding) / (pow(2, spreading)/(bandwidth/1000.0)) ) * 1000.0);

}

/*virtual*/ RadioLibInterface::~RadioLibInterface() {
	stop();
}

bool RadioLibInterface::start() {
	online(false);
	info("LoRa initializing...");
  
	SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
	delay(1500);

	// initialize SX1276 with default settings
	Serial.print(F("[SX1276] Initializing ... "));

	// carrier frequency:           915.0 MHz
	// bandwidth:                   125.0 kHz
	// spreading factor:            8
	// coding rate:                 5
	// sync word:                   0x12
	// output power:                2 dBm
	// preamble length:             20 symbols
	// amplifier gain:              1 (maximum gain)
	// FOLLOWING IS WORKING!!!
	radio = new Module(RADIO_CS_PIN, RADIO_DIO0_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);

	// initialize radio
	//int state = radio.begin(915.0, 125.0, 8, 5, 0x12, 2, 20, 1);
	//int state = radio.begin(frequency/1E6, bandwidth/1E3, spreading, coding, 0x12, power);
	//int state = radio.begin(frequency/1E6, bandwidth/1E3, spreading, coding, 0x12, power, 20, 1);
	int state = radio.begin(915.0, 125.0, 8, 5, 0x12, 7, 20);
	if (state == RADIOLIB_ERR_NONE) {
		Serial.println(F("success!"));
		radio.setOutputPower(power);
	}
	else {
		Serial.print(F("failed, code "));
		Serial.println(state);
		return false;
	}

	// set the function that will be called
	// when new packet is received
	//radio.setDio1Action(setFlag);
	#ifdef USE_SX1262
	radio.setDio1Action(setFlag);
	#else
	radio.setDio0Action(setFlag, RISING);
	#endif

	// start listening
	Serial.print(F("[SX1276] Starting to listen ... "));
	state = radio.startReceive();
	if (state != RADIOLIB_ERR_NONE) {
		operation = OPERATION_NONE;
		Serial.print(F("failed, code "));
		Serial.println(state);
		return false;
	}
	operation = OPERATION_RECEIVE;
	Serial.println(F("success!"));

	info("LoRa init succeeded.");
	extreme("LoRa bandwidth is " + std::to_string(Utilities::OS::round(bitrate()/1000.0, 2)) + " Kbps");

	online(true);
	return true;
}

void RadioLibInterface::stop() {

	online(false);
}

void RadioLibInterface::loop() {

	if (online()) {
		// Check for incoming packet

		// check if the previous operation finished
		if (operationDone) {
Serial.println(F("[SX1276] !!!!!!!!!!!!! Operation done !!!!!!!!!!!!!!!!!!!!!"));
			// reset flag
			operationDone = false;

			switch (operation) {

			case OPERATION_RECEIVE:
			{
				Serial.println(F("[SX1276] Reading packet..."));

				//String str;
				//int state = radio.readData(str);
				size_t length = radio.getPacketLength();
				//uint8_t header = 0;
				//radio.readData(&header, 1);
				//int state = radio.readData(buffer.writable(length), length-1);
				// CBA Following very double buffer copy is very inefficient
				receive_buffer.clear();
				int state = radio.readData(receive_buffer.writable(length), length);
				uint8_t header = receive_buffer.data()[0];
				receive_buffer = receive_buffer.mid(1);
				if (state == RADIOLIB_ERR_NONE) {
					// packet was successfully received
					Serial.print(F("[SX1276] Received packet of length "));
					Serial.println(length);

					// print data of the packet
					Serial.print(F("[SX1276] Data:\t\t"));
					//Serial.println(str);
					Serial.println(receive_buffer.toHex().c_str());

					// print RSSI (Received Signal Strength Indicator)
					Serial.print(F("[SX1276] RSSI:\t\t"));
					Serial.print(radio.getRSSI());
					Serial.println(F(" dBm"));

					// print SNR (Signal-to-Noise Ratio)
					Serial.print(F("[SX1276] SNR:\t\t"));
					Serial.print(radio.getSNR());
					Serial.println(F(" dB"));

					on_incoming(receive_buffer);

#ifdef HAS_DISPLAY
					if (u8g2) {
						u8g2->clearBuffer();
						//char buf[256];
						String line;
						line = "Received " + String(receive_buffer.size()) + " bytes";
						u8g2->drawStr(0, 12, line.c_str());
						//u8g2->drawStr(5, 26, str.c_str());
						//snprintf(buf, sizeof(buf), "RSSI: %.0f", LoRa.packetRssi());
						line = "RSSI: " + String(radio.getRSSI());
						u8g2->drawStr(0, 40, line.c_str());
						//snprintf(buf, sizeof(buf), "SNR: %.2f", LoRa.packetSnr());
						line = "SNR: " + String(radio.getSNR());
						u8g2->drawStr(0, 54, line.c_str());
						u8g2->sendBuffer();
					}
#endif
				}

				// CBA only start listening again if we're not already transmitting
				if (operation != OPERATION_TRANSMIT) {
					// start listening again
					Serial.println(F("[SX1276] Starting to listen again... "));
					radio.startReceive();
					operation = OPERATION_RECEIVE;
				}

				break;
			}
			case OPERATION_TRANSMIT:
			{
				// the previous operation was transmission
				if (transmissionState == RADIOLIB_ERR_NONE) {
					// packet was successfully sent
					radio.finishTransmit();
					Serial.println(F("[SX1276] Transmission finished!"));
				}
				else {
					Serial.print(F("[SX1276] Transmission failed, code "));
					Serial.println(transmissionState);
				}

				// start listening again
				Serial.println(F("[SX1276] Starting to listen again... "));
				radio.startReceive();
				operation = OPERATION_RECEIVE;

				break;
			}
			default:
				Serial.print(F("Unknown operation state!!!"));
				assert(false);
				break;

			}
		}

	}
}

/*virtual*/ void RadioLibInterface::on_incoming(const Bytes& data) {
	debug(toString() + ".on_incoming: data: " + data.toHex());
	Interface::on_incoming(data);
}

/*virtual*/ void RadioLibInterface::on_outgoing(const Bytes& data) {
	debug(toString() + ".on_outgoing: data: " + data.toHex());
	try {
		if (online()) {
			extreme("RadioLibInterface: sending " + std::to_string(data.size()) + " bytes...");
			// Send packet

			Serial.println(F("[SX1276] Sending packet ... "));

			//transmissionState = radio.startTransmit("Hello World!");

			// write header (for detecting split packets)
			uint8_t header  = Cryptography::randomnum(256) & 0xF0;
			//radio.write(header);

			// CBA TODO add support for split packets

			// add payload
			//radio.startTransmit((uint8_t*)data.data(), data.size());
			// CBA Following very double buffer copy is very inefficient
			transmit_buffer.clear();
			transmit_buffer << header;
			transmit_buffer << data;
			radio.startTransmit((uint8_t*)transmit_buffer.data(), transmit_buffer.size());

			operation = OPERATION_TRANSMIT;

			extreme("RadioLibInterface: sent bytes");

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
