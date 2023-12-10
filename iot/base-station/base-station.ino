#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LoRa.h>
#include "AESLib.h"

#define csPin 5
#define resetPin 14
#define irqPin 2

AESLib aesLib;

char cleartext[256];
char ciphertext[512];

byte localAddr = 0xAA;
byte destAddr = 0xBB;

// AES Encryption Key
byte aes_key[] = {0x33, 0x32, 0x33, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_iv[N_BLOCK] = {0xAF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAF};

byte enc_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // iv_block gets written to, provide own fresh copy...
byte dec_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const char *ssid = "CogniSafe";
const char *password = "12345678";
const char *endpoint = "http://192.168.137.1:4444/plate/verify";

static TaskHandle_t xHTTPTaskHandle = NULL;
int g_httpResponseCode = -1;

WiFiMulti wifiMulti;

QueueHandle_t rec_payloadQ;

typedef struct
{
    String user_id;
    String location;
} Payload;

void sendMessage(String outgoing);
bool receiveMessage();
String encrypt_impl(char *msg, byte iv[]);
String decrypt_impl(char *msg, byte iv[]);
void aes_init();

void receivePayload_task(void *c)
{
    while (true)
    {
        if (LoRa.parsePacket() != 0)
        {
            if (receiveMessage())
            {
                Serial.println("Message valid!");
            }
            else
                Serial.println("Message not valid");
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void taskHTTPClient(void *pvParameters)
{
    while (1)
    {
        Payload payloadData;
        /* Check if WiFi is connected and queue is not empty */
        if (wifiMulti.run() == WL_CONNECTED && xQueueReceive(rec_payloadQ, &payloadData, 5) == pdTRUE)
        {
            WiFiClient client;
            HTTPClient http;

            if (http.begin(client, endpoint))
            {
                http.addHeader("Content-Type", "application/x-www-form-urlencoded");
                char payload[128];
                sprintf(payload, "v_id=%s&location=%s", payloadData.user_id, payloadData.location);
                Serial.println(payload);
                int httpResponseCode = http.POST(payload);
                g_httpResponseCode = httpResponseCode;

                if (httpResponseCode == 200)
                {
                    Serial.println("Request Success");
                }
                else
                {
                    Serial.print("HTTP Response code: ");
                    Serial.println(httpResponseCode);
                    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
                }
                /* Send LoRa message */
                String msg_payload = String(g_httpResponseCode);
                sendMessage(msg_payload);

                Serial.print(" from 0x" + String(localAddr, HEX));
                Serial.println(" to 0x" + String(destAddr, HEX));
                g_httpResponseCode = -1;

                http.end();
            }
            else
            {
                Serial.println("Client failed!");
            }
        }
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void init_LoRa()
{
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
    size_t lenPos = decrypted.indexOf(";");
    size_t splitPos = decrypted.indexOf(",");
    size_t endPos = decrypted.indexOf("*");

    int msgLen = decrypted.substring(0, lenPos).toInt();
    String idMsg = decrypted.substring(lenPos + 1, splitPos);
    String locationMsg = decrypted.substring(splitPos + 1, endPos);
    // Serial.print(" | msgLen: ");
    // Serial.print(msgLen);
    Serial.print("msg: ");
    Serial.println(idMsg);
    Serial.print("location: ");
    Serial.println(locationMsg);

    for (int i = 0; i < 16; i++)
    {
        dec_iv[i] = 0;
    }

    if (msgLen == (idMsg.length() + locationMsg.length() + 1))
    {
        Payload payload;
        payload.location = locationMsg;
        payload.user_id = idMsg;

        xQueueSend(rec_payloadQ, &payload, portMAX_DELAY);
        return true;
    }
    else
    {
        return false;
    }
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
    aesLib.gen_iv(aes_iv);
    Serial.println(encrypt_impl(strdup(test.c_str()), aes_iv));
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(ssid, password);
    while (wifiMulti.run() != WL_CONNECTED)
    {
        Serial.println("Connecting to WiFi...");
        delay(1000);
    }
    Serial.print("Connected to WiFi in ");
    Serial.print(WiFi.localIP());
    Serial.println();

    /* AES */
    aes_init();
    aesLib.set_paddingmode(paddingMode::CMS);
    init_LoRa();

    /* FreeRTOS */
    rec_payloadQ = xQueueCreate(10, sizeof(Payload));

    while (rec_payloadQ == NULL)
    {
        Serial.println("Queue creation failed");
        rec_payloadQ = xQueueCreate(10, sizeof(Payload));
    }
    Serial.println("Queue creation success");

    // Tasks

    /*  LoRa */
    xTaskCreatePinnedToCore(receivePayload_task, "receivePayload_task", 10000, NULL, 1, NULL, 1);
    delay(200);
    xTaskCreatePinnedToCore(taskHTTPClient, "HTTPClient", 10000, NULL, 1, &xHTTPTaskHandle, 0);
}

void loop() {}
