#include <HTTPClient.h>
#include <IRremote.hpp>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ESP32Servo.h>
#include <LoRa.h>
#include "AESLib.h"

AESLib aesLib;

#define csPin 5
#define resetPin 14
#define irqPin 15

#define IR_RECEIVE_PIN 25
#define LED_BUILTIN 2 // Replace with the actual built-in LED pin
#define MOTOR_PIN 4   // Replace with the actual pin controlling the DC motor
#define SOUND_SPEED 0.034

char cleartext[256];
char ciphertext[512];

byte localAddr = 0xBB;
byte destAddr = 0xAA;

// AES Encryption Key
byte aes_key[] = {0x33, 0x32, 0x33, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30};
// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_iv[N_BLOCK] = {0xAF, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAF};

byte enc_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // iv_block gets written to, provide own fresh copy...
byte dec_iv[N_BLOCK] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const char *ssid = "CogniSafe";
const char *password = "12345678";
const char *endpoint = "http://192.168.137.1:4444/plate/verify";
Servo myservo; // create servo object to control a servo
int servoPin = 33;
const int trigPin = 32;
const int echoPin = 35;
long duration;
float distanceCm;
static TaskHandle_t xIRTaskHandle = NULL;
static TaskHandle_t xHTTPTaskHandle = NULL;
static TaskHandle_t xCallbackTaskHandle = NULL;
static TaskHandle_t xSensorTaskHandle = NULL;

String g_receivedID;
char receivedID[20];
const char location[120] = "Parkiran Digi";
bool readAgain = true;
int counter = 0;

WiFiMulti wifiMulti;

SemaphoreHandle_t httpSemaphore, receiverSemaphore;

bool useLoRa = false;
int g_httpResponseCode = -1;

String encrypt_impl(char *msg, byte iv[]);
String decrypt_impl(char *msg, byte iv[]);
void aes_init();
void taskIRReceiver(void *pvParameters);
void taskHTTPClient(void *pvParameters);
void onRequestSuccess(void *pvParameters);
void vSensorTask(void *pvParam);
void init_LoRa();
void sendMessage(String outgoing);
bool receiveMessage();

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

void taskIRReceiver(void *pvParameters)
{
  int delayInterval = 0;
  int pos;
  while (1)
  {
    if (distanceCm <= 5)
    {
      if (IrReceiver.decode() && readAgain)
      {
        if (IrReceiver.decodedIRData.protocol == UNKNOWN)
        {
          Serial.printf("Received noise or an unknown protocol wait 1 seconds %d\n", counter++);
          delayInterval = 1;
        }
        else
        {
          sprintf(receivedID, "VID%08X", IrReceiver.decodedIRData.decodedRawData);
          Serial.printf("%s\n", receivedID);
          xSemaphoreGive(receiverSemaphore);
          delayInterval = 1000;
        }
        IrReceiver.resume();
      }
    }
    else
    {
      xSemaphoreTake(receiverSemaphore, 0);
      delay(delayInterval);
      vTaskResume(xHTTPTaskHandle);
      pos = myservo.read();
      if (pos > 90)
      {
        pos = 0;
      }
      for (pos; pos >= 0; pos -= 1)
      { // goes from 0 degrees to 180 degrees
        // in steps of 1 degree
        myservo.write(pos); // tell servo to go to position in variable 'pos'
        delay(5);           // waits 15 ms for the servo to reach the position
      }                     // vTaskSuspend(xIRTaskHandle);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void taskHTTPClient(void *pvParameters)
{
  while (1)
  {
    if (wifiMulti.run() == WL_CONNECTED && xSemaphoreTake(receiverSemaphore, portMAX_DELAY) == pdTRUE && !useLoRa)
    {
      WiFiClient client;
      HTTPClient http;

      if (http.begin(client, endpoint))
      {
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        char payload[128];
        sprintf(payload, "v_id=%s&location=%s", receivedID, location);
        Serial.println(payload);
        readAgain = false;
        int httpResponseCode = http.POST(payload);

        if (httpResponseCode == 200)
        {
          Serial.println("Request Success");
          xSemaphoreGive(httpSemaphore); // Trigger other tasks
          readAgain = false;
          vTaskSuspend(xHTTPTaskHandle);
        }
        else
        {
          // Use LoRa
          useLoRa = true;
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
          readAgain = true;
        }

        http.end();
      }
      else
      {
        Serial.println("Client failed!");
      }
    }

    else if (xSemaphoreTake(receiverSemaphore, portMAX_DELAY) == pdTRUE && useLoRa)
    {
      // Generate msg_payload
      String msg_payload = String(receivedID) + "," + String(location);

      /* BLOCKING CODE BELOW */
      short n_retry = 3;
      short retry = n_retry;
      /*
      This while loop will try to access backend via LoRa first
      Will try to resend the message if the response is not valid
      or the response code is not 200
      (max n_retry [3] number of times)
      */
      while (retry-- > 0)
      {
        Serial.print("Trying to make request (");
        Serial.print(n_retry - retry);
        Serial.println(" of ");
        Serial.print(n_retry);
        Serial.println(")");

        bool success = false;
        long timeout = millis() + 3000; // 3 seconds of timeout to read message
        while (!success)
        {
          // Send message
          sendMessage(msg_payload);
          // Wait for response
          while (LoRa.parsePacket() == 0 && millis() < timeout)
          {
          };
          // If above timeout -> resend
          if (LoRa.parsePacket() != 0)
          {
            // If response is valid, will return true
            // Even if the response code is not 200
            success = receiveMessage();
          }
        }

        // Check response
        if (success && g_httpResponseCode == 200)
        {
          // If response code is 200, give semaphore for onRequestSuccess
          Serial.println("Request Success");
          xSemaphoreGive(httpSemaphore);
          // Suspends this task
          vTaskSuspend(xHTTPTaskHandle);
          // No need to resend
          break;
        }
        // If response code is not 200, resend
        else if (!success)
        {
          // If response is not valid, will return false
          Serial.println("Message is not valid");
        }
        else
        {
          Serial.println("HTTP Request failed");
        }
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void onRequestSuccess(void *pvParameters)
{
  int pos;
  while (1)
  {
    if (xSemaphoreTake(httpSemaphore, portMAX_DELAY) == pdTRUE)
    {
      digitalWrite(LED_BUILTIN, HIGH);      // Turn on the LED
      vTaskDelay(300 / portTICK_PERIOD_MS); // Keep the LED on for 1 second
      digitalWrite(LED_BUILTIN, LOW);       // Turn off the LED

      for (pos = 0; pos <= 90; pos += 1)
      { // goes from 0 degrees to 180 degrees
        // in steps of 1 degree
        myservo.write(pos); // tell servo to go to position in variable 'pos'
        delay(5);           // waits 15 ms for the servo to reach the position
      }                     // vTaskSuspend(xIRTaskHandle);

      readAgain = true;
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void vSensorTask(void *pvParam)
{
  while (1)
  {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculate the distance
    distanceCm = duration * SOUND_SPEED / 2;
    Serial.println(distanceCm);
    vTaskDelay(pdMS_TO_TICKS(100)); // memberikan delay
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
  //
  String msgToSend = String(outgoing.length()) + String(";") + outgoing + String("*");
  sprintf(cleartext, "%s", msgToSend.c_str()); // must not exceed 255 bytes; may contain a newline
  // Encrypt
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
  // Read message
  String incoming = LoRa.readStringUntil('\n');
  Serial.println(incoming);
  // Convert to char array
  sprintf(ciphertext, "%s", incoming.c_str());
  // Decrypt
  String decrypted = decrypt_impl(ciphertext, dec_iv);
  // Parse message
  size_t lenPos = decrypted.indexOf(";");
  size_t endPos = decrypted.indexOf("*");
  // msgLen -> length of message, for validation
  int msgLen = decrypted.substring(0, lenPos).toInt();
  String msg = decrypted.substring(lenPos + 1, endPos);
  // msg -> status code http

  Serial.print("msg (");
  Serial.print(msgLen);
  Serial.print(") : ");
  Serial.println(msg);

  for (int i = 0; i < 16; i++)
  {
    dec_iv[i] = 0;
  }

  // If msgLen is equal to the length of the message
  // Then the message is valid
  if (msgLen == msg.length())
  {

    g_httpResponseCode = msg.toInt();
    return true;
  }
  else
    return false;
}

void setup()
{
  int retry = 5;
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(IR_RECEIVE_PIN, INPUT);
  myservo.setPeriodHertz(100); // standard 50 hz servo
  myservo.attach(servoPin);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  while (wifiMulti.run() != WL_CONNECTED)
  {
    if (retry-- == 0)
    {
      useLoRa = true;
      Serial.println("Failed to connect to WiFi");
      break;
    }
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  if (!useLoRa)
  {
    Serial.print("Connected to WiFi in ");
    Serial.print(WiFi.localIP());
    Serial.println();
  }
  else
  {
    Serial.println("Using LoRa");
  }

  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  httpSemaphore = xSemaphoreCreateBinary();
  receiverSemaphore = xSemaphoreCreateBinary();

  /* AES */
  aes_init();
  aesLib.set_paddingmode(paddingMode::CMS);
  init_LoRa();

  /* FreeRTOS */
  xTaskCreatePinnedToCore(taskHTTPClient, "HTTPClient", 10000, NULL, 1, &xHTTPTaskHandle, 0);
  xTaskCreatePinnedToCore(taskIRReceiver, "IRReceiver", 10000, NULL, 1, &xIRTaskHandle, 1);
  xTaskCreatePinnedToCore(onRequestSuccess, "RequestSuccessCallback", 10000, NULL, 1, &xCallbackTaskHandle, 0);
  xTaskCreatePinnedToCore(vSensorTask, "SensorTask", 10000, NULL, 2, &xSensorTaskHandle, 1);
}

void loop() {}
