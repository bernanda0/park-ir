#include <Arduino.h>
#include <IRremote.hpp>
#include "PinDefinitionsAndMore.h"  // Define macros for input and output pin etc.

#define IR_SEND_PIN         25
#define IR_SEND_PIN_STRING "25"
#define DISABLE_CODE_FOR_RECEIVER // Disables restarting receiver after each send. Saves 450 bytes program memory and 269 bytes RAM if receiving functions are not used.
#define DELAY_AFTER_SEND 3000

// this hardcoded, example VID958F6538
uint32_t plateID = 0x958F6538;

void setup() {
    Serial.begin(115200);

#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
    IrSender.begin(); // Start with IR_SEND_PIN as send pin and enable feedback LED at default feedback LED pin
    Serial.println(F("Send IR signals at pin " IR_SEND_PIN_STRING));
    Serial.println(F("Send IR signals at pin " STR(IR_SEND_PIN)));
}


void loop() {
    Serial.printf("Send NECRaw(%s)\n", plateID);
    IrSender.sendNECRaw(plateID, 0);
    delay(DELAY_AFTER_SEND);
}
