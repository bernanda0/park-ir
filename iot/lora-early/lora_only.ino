#include <SPI.h>
#include <LoRa.h>
#include "AESLib.h"

#define csPin 5
#define resetPin 14
#define irqPin 2

AESLib aesLib;

typedef struct payload_t
{
  bool valid;
  String id;
  String location;
} payload_t;

char cleartext[256];
char ciphertext[512];

byte localAddr = 0xBB;
byte destAddr = 0xAA;
long lastSendTime = 0;
int interval = 2000;
int count = 0;

// FreeRTOS stuff
static QueueHandle_t rec_payloadQ;
static QueueHandle_t send_payloadQ;
SemaphoreHandle_t xLoRaMutex = NULL;

// AES Encryption Key
byte aes_key[] = {0x33, 0x32, 0x33, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_iv[N_BLOCK] = {0xAF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAF};

unsigned long loopcount = 0;
byte enc_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // iv_block gets written to, provide own fresh copy...
byte dec_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Prototypes */
void sendMessage(String outgoing);
bool receiveMessage();
void init_LoRa();
String encrypt_impl(char *msg, byte iv[]);
String decrypt_impl(char *msg, byte iv[]);
void aes_init();

void receivePayload_task(void *c)
{
  while (true)
  {
    payload_t payload;
    // if (xQueueReceive(rec_payloadQ, &payload, 50 / portTICK_PERIOD_MS))
    // {
    //   Serial.println("Received payload");
    //   Serial.println(payload.id);
    //   Serial.println(payload.location);
    // }
    if (LoRa.parsePacket() != 0)
    {
      if (receiveMessage())
        Serial.println("Message valid!");
      else
        Serial.println("Message not valid");
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

void sendPayload_task(void *pvParams)
{
  while (true)
  {
    // Serial.println("Send START");
    String sensorData = String("idbuatdikirim;");
    // Serial.print("Sending data: " + sensorData);
    sendMessage(sensorData);

    Serial.print(" from 0x" + String(localAddr, HEX));
    Serial.println(" to 0x" + String(destAddr, HEX));

    // Serial.println("Send END");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  xLoRaMutex = xSemaphoreCreateMutex();
  while (!Serial)
    ; // Wait for Serial to be ready

  /* AES */
  aes_init();
  aesLib.set_paddingmode(paddingMode::CMS);
  init_LoRa();

  /* FreeRTOS */
  // rec_payloadQ = xQueueCreate(10, sizeof(String));
  // Tasks
  xTaskCreatePinnedToCore(receivePayload_task, "receivePayload_task", 10000, NULL, 1, NULL, 1);
  delay(200);
  // xTaskCreatePinnedToCore(sendPayload_task, "sendPayload_task", 10000, NULL, 1, NULL, 1);
  delay(200);
  /*  LoRa */

  vTaskDelete(NULL);
}

void loop() {}

void init_LoRa()
{
  // Serial.print("\n\n[LoRa] Initializing {");
  // Serial.print("localAddr: ");
  // Serial.print(localAddr, HEX);
  // Serial.print("destAddr: ");
  // Serial.print(destAddr, HEX);
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(433E6))
  {
    Serial.println("[LoRa] Init failed");
    while (true)
    {
    }
  }

  LoRa.setSyncWord(0xF3);
  LoRa.setTxPower(20);
  LoRa.setCodingRate4(5);
  Serial.println("[LoRa] Init success");
}

void sendMessage(String outgoing)
{
  String msgToSend = String(outgoing.length()) + String(';') + outgoing + String('*');
  sprintf(cleartext, "%s", msgToSend.c_str()); // must not exceed 255 bytes; may contain a newline
  // Encrypt
  Serial.println("encrypt_impl()");
  String encrypted = encrypt_impl(cleartext, enc_iv);
  LoRa.beginPacket();
  // LoRa.write(destAddr);
  // LoRa.write(localAddr);
  // LoRa.write(outgoing.length());
  // LoRa.println(encrypted);
  LoRa.println(encrypted);
  LoRa.endPacket();

  Serial.print("Sending data " + encrypted);

  for (int i = 0; i < 16; i++)
  {
    enc_iv[i] = 0;
  }
}

bool receiveMessage()
{
  String incoming = LoRa.readStringUntil('\n');

  Serial.println(incoming);
  sprintf(ciphertext, "%s", incoming.c_str());

  String decrypted = decrypt_impl(ciphertext, dec_iv);
  // decrypted = decrypted.substring(0, incoming.length());

  // Serial.print(" | ");
  // Serial.print(decrypted.length());
  // Serial.print(" : ");
  // Serial.println(decrypted);

  size_t splitPos = decrypted.indexOf(";");
  size_t endPos = decrypted.indexOf("*");

  int msgLen = decrypted.substring(0, splitPos).toInt();
  String msg = decrypted.substring(splitPos + 1, endPos);
  // Serial.print(" | msgLen: ");
  // Serial.print(msgLen);
  Serial.print("msg: ");
  Serial.println(msg);

  for (int i = 0; i < 16; i++)
  {
    dec_iv[i] = 0;
  }

  if (msgLen == msg.length())
    return true;
  else
    return false;
}

String encrypt_impl(char *msg, byte iv[])
{
  int msgLen = strlen(msg);
  char encrypted[2 * msgLen] = {0};
  aesLib.encrypt64((const byte *)msg, msgLen, encrypted, aes_key, sizeof(aes_key), iv);
  return String(encrypted);
}

String decrypt_impl(char *msg, byte iv[])
{
  int msgLen = strlen(msg);
  char decrypted[msgLen] = {0}; // half may be enough
  aesLib.decrypt64(msg, msgLen, (byte *)decrypted, aes_key, sizeof(aes_key), iv);
  return String(decrypted);
}

// Generate IV (once)
void aes_init()
{
  String test = "HELLO WORLD!";
  Serial.println("gen_iv()");
  aesLib.gen_iv(aes_iv);
  Serial.println("encrypt_impl()");
  Serial.println(encrypt_impl(strdup(test.c_str()), aes_iv));
}