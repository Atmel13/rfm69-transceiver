#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>
#include <RFM69.h>

#define RF_CS   7 // D7 on ESP32-C3 -> NSS on RFM69
#define RF_IRQ  3 // D3 on ESP32-C3 -> DIO0 on RFM69

#define NODE_ID     1 // unique ID of this node (1-254), also used as destination ID for sending packets
#define NETWORK_ID  0 // all nodes on the same network should have the same network ID, not used for routing but must match the one in the RFM69

extern RFM69 radio;

void setupRadio();

#endif