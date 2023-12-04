#include <Reticulum.h>
#include <Identity.h>
#include <Destination.h>
#include <Packet.h>
#include <Transport.h>
#include <Interface.h>
#include <Log.h>
#include <Bytes.h>
#include <Type.h>
#include <Interfaces/UDPInterface.h>
#include <Utilities/OS.h>

#include "LoRaInterface.h"

#include <Arduino.h>
#include "variants/config.h"
#include "variants/init.h"

#include <unistd.h>
#include <string>
#include <functional>


//#define UDP_INTERFACE
#define LORA_INTERFACE
//#define TEST_DESTINATIONS


// Let's define an app name. We'll use this for all
// destinations we create. Since this basic example
// is part of a range of example utilities, we'll put
// them all within the app namespace "example_utilities"
const char* APP_NAME = "MCU";

// We initialise two lists of strings to use as app_data
const char* fruits[] = {"Peach", "Quince", "Date", "Tangerine", "Pomelo", "Carambola", "Grape"};
const char* noble_gases[] = {"Helium", "Neon", "Argon", "Krypton", "Xenon", "Radon", "Oganesson"};

double last_announce = 0.0;
bool send_announce = false;

// Test AnnounceHandler
class ExampleAnnounceHandler : public RNS::AnnounceHandler {
public:
	ExampleAnnounceHandler(const char* aspect_filter = nullptr) : AnnounceHandler(aspect_filter) {}
	virtual ~ExampleAnnounceHandler() {}
	virtual void received_announce(const RNS::Bytes& destination_hash, const RNS::Identity& announced_identity, const RNS::Bytes& app_data) {
		RNS::info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		RNS::info("ExampleAnnounceHandler: destination hash: " + destination_hash.toHex());
		if (announced_identity) {
			RNS::info("ExampleAnnounceHandler: announced identity hash: " + announced_identity.hash().toHex());
			RNS::info("ExampleAnnounceHandler: announced identity app data: " + announced_identity.app_data().toHex());
		}
        if (app_data) {
			RNS::info("ExampleAnnounceHandler: app data text: \"" + app_data.toString() + "\"");
		}
		RNS::info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	}
};

// Test packet receive callback
void onPacket(const RNS::Bytes& data, const RNS::Packet& packet) {
	RNS::info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	RNS::info("onPacket: data: " + data.toHex());
	RNS::info("onPacket: text: \"" + data.toString() + "\"");
	//RNS::extreme("onPacket: " + packet.debugString());
	RNS::info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

#ifdef HAS_DISPLAY
	if (u8g2) {
		u8g2->clearBuffer();
		String line;
		u8g2->drawStr(0, 12, "Received DATA...");
		u8g2->drawStr(5, 26, data.toString().c_str());
		line = "RSSI: " + String(LoRa.packetRssi());
		u8g2->drawStr(0, 40, line.c_str());
		line = "SNR: " + String(LoRa.packetSnr());
		u8g2->drawStr(0, 54, line.c_str());
		u8g2->sendBuffer();
	}
#endif
}

// Ping packet receive callback
void onPingPacket(const RNS::Bytes& data, const RNS::Packet& packet) {
	RNS::info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
	RNS::info("onPingPacket: data: " + data.toHex());
	RNS::info("onPingPacket: text: \"" + data.toString() + "\"");
	//RNS::extreme("onPingPacket: " + packet.debugString());
	RNS::info("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

#ifdef HAS_DISPLAY
	if (u8g2) {
		u8g2->clearBuffer();
		String line;
		u8g2->drawStr(0, 12, "Received PING...");
		u8g2->drawStr(5, 26, data.toString().c_str());
		line = "RSSI: " + String(LoRa.packetRssi());
		u8g2->drawStr(0, 40, line.c_str());
		line = "SNR: " + String(LoRa.packetSnr());
		u8g2->drawStr(0, 54, line.c_str());
		u8g2->sendBuffer();
	}
#endif
}


RNS::Reticulum reticulum({RNS::Type::NONE});
RNS::Identity identity({RNS::Type::NONE});
RNS::Destination destination({RNS::Type::NONE});

#ifdef UDP_INTERFACE
RNS::Interfaces::UDPInterface udp_interface;
#endif
#ifdef LORA_INTERFACE
LoRaInterface lora_interface;
#endif

//ExampleAnnounceHandler announce_handler((const char*)"example_utilities.announcesample.fruits");
//RNS::HAnnounceHandler announce_handler(new ExampleAnnounceHandler("example_utilities.announcesample.fruits"));
RNS::HAnnounceHandler announce_handler(new ExampleAnnounceHandler());

void reticulum_announce() {
	if (destination) {
		RNS::head("Announcing destination...", RNS::LOG_EXTREME);
		//destination.announce(RNS::bytesFromString(fruits[RNS::Cryptography::randomnum() % 7]));
		// test path
		//destination.announce(RNS::bytesFromString(fruits[RNS::Cryptography::randomnum() % 7]), true, nullptr, RNS::bytesFromString("test_tag"));
		// test packet send
		destination.announce(RNS::bytesFromString(fruits[RNS::Cryptography::randomnum() % 7]));
	}
}

void reticulum_setup() {
	RNS::info("Setting up Reticulum...");

	try {

		RNS::head("Creating Reticulum instance...", RNS::LOG_EXTREME);
		reticulum = RNS::Reticulum();

		RNS::head("Registering Interface instances with Transport...", RNS::LOG_EXTREME);
#ifdef UDP_INTERFACE
		udp_interface.mode(RNS::Type::Interface::MODE_GATEWAY);
		RNS::Transport::register_interface(udp_interface);
#endif
#ifdef LORA_INTERFACE
		lora_interface.mode(RNS::Type::Interface::MODE_GATEWAY);
		RNS::Transport::register_interface(lora_interface);
#endif

#ifdef UDP_INTERFACE
		RNS::head("Starting UDPInterface...", RNS::LOG_EXTREME);
		udp_interface.start("some_ssid", "some_password");
#endif

#ifdef LORA_INTERFACE
		RNS::head("Starting LoRaInterface...", RNS::LOG_EXTREME);
		lora_interface.start();
#endif

		RNS::head("Creating Identity instance...", RNS::LOG_EXTREME);

		// new identity
		//RNS::Identity identity;
		//identity = RNS::Identity();

		// predefined identity (obviously insecure, but only until random keys can be persisted)
		// hash:          B5F5500B44B99153B684CC3B4365AB13
		// prv_bytes:     A8AF6C2F9A063BACAC90C8CE7FA436ECF02439151B5787BB3F3313D192D13875
		// sig_prv_bytes: 569A90CBAC2F4CAE21406802C21F40991A58F52C2315A867E1AF54D76B27AB90
		//RNS::Identity identity(false);
		identity = RNS::Identity(false);
		RNS::Bytes prv_bytes;
		prv_bytes.assignHex("A8AF6C2F9A063BACAC90C8CE7FA436ECF02439151B5787BB3F3313D192D13875569A90CBAC2F4CAE21406802C21F40991A58F52C2315A867E1AF54D76B27AB90");
		identity.load_private_key(prv_bytes);

#ifdef TEST_DESTINATIONS
		RNS::head("Creating Destination instance...", RNS::LOG_EXTREME);
		destination = RNS::Destination(identity, RNS::Type::Destination::IN, RNS::Type::Destination::SINGLE, APP_NAME, "test");

		RNS::head("Registering packet callback with Destination...", RNS::LOG_EXTREME);
		destination.set_packet_callback(onPacket);
		destination.set_proof_strategy(RNS::Type::Destination::PROVE_ALL);

		// Destination for Echo example
		{
			RNS::head("Creating PING Destination instance...", RNS::LOG_EXTREME);
			RNS::Destination ping_destination(identity, RNS::Type::Destination::IN, RNS::Type::Destination::SINGLE, "example_utilities", "echo.request");

			RNS::head("Registering packet callback with PING Destination...", RNS::LOG_EXTREME);
			ping_destination.set_packet_callback(onPingPacket);
			ping_destination.set_proof_strategy(RNS::Type::Destination::PROVE_ALL);
		}
#endif

		RNS::head("Registering announce handler with Transport...", RNS::LOG_EXTREME);
		RNS::Transport::register_announce_handler(announce_handler);

		RNS::head("Ready!", RNS::LOG_EXTREME);
	}
	catch (std::exception& e) {
		RNS::error(std::string("!!! Exception in reticulum_setup: ") + e.what() + " !!!");
	}
}

void reticulum_teardown() {
	RNS::info("Tearing down Reticulum...");

	try {

		RNS::head("Deregistering announce handler with Transport...", RNS::LOG_EXTREME);
		RNS::Transport::deregister_announce_handler(announce_handler);

		RNS::head("Deregistering Interface instances with Transport...", RNS::LOG_EXTREME);
#ifdef UDP_INTERFACE
		RNS::Transport::deregister_interface(udp_interface);
#endif
#ifdef LORA_INTERFACE
		RNS::Transport::deregister_interface(lora_interface);
#endif

	}
	catch (std::exception& e) {
		RNS::error(std::string("!!! Exception in reticulum_teardown: ") + e.what() + " !!!");
	}
}

void userKey(void)
{
	if (digitalRead(BUTTON_PIN) == LOW) {
		send_announce = true;
	}
}

void setup() {

	//Serial.begin(115200);
	initBoard();

	// Setup user button
	pinMode(BUTTON_PIN, INPUT);
	attachInterrupt(BUTTON_PIN, userKey, FALLING);  

	RNS::loglevel(RNS::LOG_EXTREME);

	reticulum_setup();
}

void loop() {

	reticulum.loop();

#ifdef UDP_INTERFACE
	udp_interface.loop();
#endif
#ifdef LORA_INTERFACE
	lora_interface.loop();
#endif

	if (send_announce) {
		reticulum_announce();
		send_announce = false;
	}

}
