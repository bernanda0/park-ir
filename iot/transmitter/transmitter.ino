#include <Arduino.h>
#include <IRremote.hpp>
#include "PinDefinitionsAndMore.h"  // Define macros for input and output pin etc.
#include <BluetoothSerial.h>

#define IR_SEND_PIN         25
#define IR_SEND_PIN_STRING "25"
#define DISABLE_CODE_FOR_RECEIVER // Disables restarting receiver after each send. Saves 450 bytes program memory and 269 bytes RAM if receiving functions are not used.
#define DELAY_AFTER_SEND 1000

// this hardcoded, example VID958F6538
uint32_t plateID = 0;
BluetoothSerial SerialBT; //variabel untuk serial monitor BT
bool BT_isOn = false; //Boolean untuk menandakan BT sudah on atau belum
bool BT_isConnected = false; //Boolean untuk menandakan BT sudah terhubung atau tidak
bool btn_toggle = false;
int btn = 32;

//Class callback yang akan diassign ke SerialBT
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    if (event == ESP_SPP_SRV_OPEN_EVT) { //Jika tersambung
        BT_isConnected = true; //mengubah boolean menjadi true
        Serial.println("Bluetooth tersambung");
    } else if (event == ESP_SPP_CLOSE_EVT) { //jika terputus
        BT_isConnected = false; //mengubah boolean menjadi false
        Serial.println("Bluetooth terputus");
    }
}

void vButtonRead(void *pvParam){
  while(1){
    if(digitalRead(btn) == LOW){
      if(btn_toggle == false){
        btn_toggle = true;
        Serial.println("toggle on");
        delay(50);
      }else{
        btn_toggle = false;
        Serial.println("toggle off");
        delay(50);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

  }
}
void vBTinit(void *pvParam){
  while(1){
    if(btn_toggle == true && BT_isOn == false){
      //Memulai SerialBT
      SerialBT.begin("esp2");
      BT_isOn = true; //mengubah boolean menjadi true
      Serial.println("Bluetooth telah dimulai");
    
    }else if(btn_toggle == false && BT_isOn == true){
      //SerialBT.end();
      BT_isOn = false;
      Serial.println("Bluetooth telah dihentikan");

    }
     vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void vBTtask(void *pvParam){
  String command;
  while(1){
    if(BT_isConnected == true){
      vTaskDelay(pdMS_TO_TICKS(50));
      //Jika SerialBT tidak tersedia
      if (SerialBT.available() <= 0){
        continue; //skip program dibawahnya
      }
      command = SerialBT.readStringUntil('\n');
      Serial.println(command);
      plateID = strtoul(command.c_str(), NULL, 16);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

  }
}

void setup() {
    Serial.begin(115200);
    pinMode(btn, INPUT_PULLUP);
    SerialBT.register_callback(callback);

  #if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
      delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
  #endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
    IrSender.begin(IR_SEND_PIN); // Start with IR_SEND_PIN as send pin and enable feedback LED at default feedback LED pin
    Serial.println(F("Send IR signals at pin " IR_SEND_PIN_STRING));
    Serial.println(F("Send IR signals at pin " STR(IR_SEND_PIN)));

    xTaskCreatePinnedToCore(vBTinit, "BT init", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(vButtonRead, "button read", 2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(vBTtask, "SerialBT read", 2048, NULL, 1, NULL, 0);
}


void loop() {
  if(plateID != 0){
    Serial.print("Sending data: ");
    Serial.printf("%x\n",plateID);
    IrSender.sendNECRaw(plateID, 0);
    delay(DELAY_AFTER_SEND);
  }
}
