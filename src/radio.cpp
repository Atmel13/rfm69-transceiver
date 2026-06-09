#include "radio.h"

RFM69 radio(RF_CS, RF_IRQ, true);

void setupRadio()
{
    pinMode(RF_IRQ, INPUT);
    pinMode(RF_CS, OUTPUT);
    digitalWrite(RF_CS, HIGH);

    radio.initialize(RF69_433MHZ, NODE_ID, NETWORK_ID);

    radio.encrypt("1234567890ABCDEF");

    radio.setHighPower();
    radio.setPowerLevel(31);

    //radio.writeReg(0x37, radio.readReg(0x37) & 0xEF);

    // BitRate = 100 kbps ------------------------------
    radio.writeReg(0x03, 0x01);
    radio.writeReg(0x04, 0x40);

    // Frequency deviation = 50 kHz
    radio.writeReg(0x05, 0x03);
    radio.writeReg(0x06, 0x33);

    // RX bandwidth ≈ 125 kHz
    radio.writeReg(0x19, 0x42);

    // AFC bandwidth ≈ 250 kHz
    radio.writeReg(0x1A, 0x8A);

    /*
    // BitRate ≈ 150.2 kbps ------------------------------
    radio.writeReg(0x03, 0x00);
    radio.writeReg(0x04, 0xD5);

    // Frequency deviation = 75 kHz
    radio.writeReg(0x05, 0x04);
    radio.writeReg(0x06, 0xCD);

    // RX bandwidth ≈ 250 kHz
    radio.writeReg(0x19, 0x41);

    // AFC bandwidth ≈ 333 кГц
    radio.writeReg(0x1A, 0x89);
    

    // BitRate = 200 kbps ------------------------------
    radio.writeReg(0x03, 0x00);
    radio.writeReg(0x04, 0xA0);

    // Frequency deviation = 100 kHz
    radio.writeReg(0x05, 0x06);
    radio.writeReg(0x06, 0x66);

    // RX bandwidth ≈ 250 kHz
    radio.writeReg(0x19, 0x41);

    // AFC bandwidth ≈ 500 kHz
    radio.writeReg(0x1A, 0x81);


    // BitRate = 250 kbps ------------------------------
    radio.writeReg(0x03, 0x00);
    radio.writeReg(0x04, 0x80);

    // Frequency deviation = 125 kHz
    radio.writeReg(0x05, 0x08);
    radio.writeReg(0x06, 0x00);

    // RX bandwidth ≈ 500 kHz
    radio.writeReg(0x19, 0x81);

    // AFC bandwidth ≈ 500 kHz
    radio.writeReg(0x1A, 0x81);

    // BitRate ≈ 299 kbps ------------------------------
    radio.writeReg(0x03, 0x00);
    radio.writeReg(0x04, 0x6B);

    // Frequency deviation = 150 kHz
    radio.writeReg(0x05, 0x09);
    radio.writeReg(0x06, 0x9A);

    // Максимальная RX BW
    radio.writeReg(0x19, 0x80);

    // Максимальная AFC BW
    radio.writeReg(0x1A, 0x80);
    */
}
