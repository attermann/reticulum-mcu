# reticulum-mcu
WIP: Experimental firmware for MCU implementing a simple Reticulum LoRa Transport node.

## Capability
Currently this firmware should enable a MCU to be operated as a basic standalone LoRa repeater node in a Reticulum network.

Please note that this firmware is experimental, incomplete, and in some cases untested. The API (reticulum-cpp) remains in major flux so expect it to change often and potentially break projects thare are derived from this.

Also note that there is currently zero persistence in the firmware so all routing information will be lost and need to be re-established on board reset. Persistence is probably the next feature of focus to make this more usable.

## Compatibility
Currently only configured for and tested on a LilyGo T-Beam v1.1 board. Support for some other LilyGo LoRa boards is likely trivial with the addition of a PlatformIO build profile and changing of the board selection in `src/variants/config.h`, but as of yet no other variants have been tested.

## Configuration
- Config for a particule board varient is selected by n-commenting the relevant #define in `src/variants/config.h`.
- UDP Interface (to test network bridging) can be enabled by un-commenting `#define UDP_INTERFACE` in `main.cpp` and specifying WiFi SSID and passowrd in the call to `udp_interface.start()`.
- Log verbosity is controlled through the call to `RNS::loglevel()`.
