#include "radio.h"

#include <RFM69registers.h>
 
RFM69 radio(RF_CS, RF_IRQ, true);
 
// Флаги (перечисление) для поддерживаемых скоростей
enum RadioSpeed {
    SPEED_100kbps = 100,
    SPEED_150kbps = 150,
    SPEED_200kbps = 200,
    SPEED_250kbps = 250,
    SPEED_300kbps = 300
};

// Функция установки скорости и сопутствующих параметров радио
void setRFM69Speed(uint16_t speed) {
    switch (speed) {
        case SPEED_100kbps:
            // BitRate = 100 kbps
            radio.writeReg(REG_BITRATEMSB, 0x01);
            radio.writeReg(REG_BITRATELSB, 0x40);
            // Frequency deviation = 50 kHz
            radio.writeReg(REG_FDEVMSB, 0x03);
            radio.writeReg(REG_FDEVLSB, 0x33);
            // RX bandwidth ≈ 125 kHz
            radio.writeReg(REG_RXBW, 0x42);
            // AFC bandwidth ≈ 250 kHz
            radio.writeReg(REG_AFCBW, 0x8A);
            break;

        case SPEED_150kbps:
            // BitRate ≈ 150.2 kbps
            radio.writeReg(REG_BITRATEMSB, 0x00);
            radio.writeReg(REG_BITRATELSB, 0xD5);
            // Frequency deviation = 75 kHz
            radio.writeReg(REG_FDEVMSB, 0x04);
            radio.writeReg(REG_FDEVLSB, 0xCD);
            // RX bandwidth ≈ 250 kHz
            radio.writeReg(REG_RXBW, 0x41);
            // AFC bandwidth ≈ 333 kHz
            radio.writeReg(REG_AFCBW, 0x89);
            break;

        case SPEED_200kbps:
            // BitRate = 200 kbps
            radio.writeReg(REG_BITRATEMSB, 0x00);
            radio.writeReg(REG_BITRATELSB, 0xA0);
            // Frequency deviation = 100 kHz
            radio.writeReg(REG_FDEVMSB, 0x06);
            radio.writeReg(REG_FDEVLSB, 0x66);
            // RX bandwidth ≈ 250 kHz
            radio.writeReg(REG_RXBW, 0x41);
            // AFC bandwidth ≈ 500 kHz
            radio.writeReg(REG_AFCBW, 0x81);
            break;

        case SPEED_250kbps:
            // BitRate = 250 kbps
            radio.writeReg(REG_BITRATEMSB, 0x00);
            radio.writeReg(REG_BITRATELSB, 0x80);
            // Frequency deviation = 125 kHz
            radio.writeReg(REG_FDEVMSB, 0x08);
            radio.writeReg(REG_FDEVLSB, 0x00);
            // RX bandwidth ≈ 500 kHz
            radio.writeReg(REG_RXBW, 0x81);
            // AFC bandwidth ≈ 500 kHz
            radio.writeReg(REG_AFCBW, 0x81);
            break;

        case SPEED_300kbps:
            // BitRate ≈ 299 kbps
            radio.writeReg(REG_BITRATEMSB, 0x00);
            radio.writeReg(REG_BITRATELSB, 0x6B);
            // Frequency deviation = 150 kHz
            radio.writeReg(REG_FDEVMSB, 0x09);
            radio.writeReg(REG_FDEVLSB, 0x9A);
            // Максимальная RX BW
            radio.writeReg(REG_RXBW, 0x80);
            // Максимальная AFC BW
            radio.writeReg(REG_AFCBW, 0x80);
            break;

        default:
            // Обработка ситуации, если передана неверная скорость
            // Можно оставить пустым, вывести ошибку или установить скорость по умолчанию (например, 100)
            break;
    }
}

void setupRadio()
{
    pinMode(RF_IRQ, INPUT);
    pinMode(RF_CS, OUTPUT);
    digitalWrite(RF_CS, HIGH);
 
    radio.initialize(RF69_433MHZ, NODE_ID, NETWORK_ID);
 
    radio.encrypt("1234567890ABCDEF");
 
    radio.setHighPower();
    radio.setPowerLevel(31);
 
    setRFM69Speed(SPEED_100kbps);
}