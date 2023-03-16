/*

TITRE          : Laboratoire 1
AUTEUR         : Franck Nkeubou Awougang
DATE           : 16/03/2023
DESCRIPTION    : Programme du laboratoire 1 qui consiste à connecter un arduino (esp32) avec OpenHAB
                  afin de gérer deux capteurs et deux actuateurs digitaux et analogues chaques.
VERSION        : 0.0.1

*/

//Librairies needed
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "Timer.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define PIN_LED 18     // Control signal, connect to DI of the LED
#define NUM_LED 1     // Number of LEDs in a strip

//Wifi credentials
//const char* ssid = "FibreOP532";
const char* ssid = "MSI";
//const char* password = "9PXPE66PM6XM55M8";
const char* password = "12345678";
// MQTT credentials
const char* mqtt_server = "192.168.2.75";
const char* mqtt_username = "ubuntu";
const char* mqtt_password = "ubuntu";

// Let set our topic to interact with OpenHAB
const char* chaine1 = "actuateur/analog";
const char* chaine2 = "actuateur/digital";
const char* chaine3 = "capteur/digitale";
const char* chaine4 = "capteur/analog";

WiFiClient espClient;
PubSubClient client(espClient);
Timer Temps;
//Global variables
int PinMoteur = 26;
uint PinBouton = 22;
int PinRotation = 33;
short int MotorSpeed = 0, wait = 1000;
int PinLED = 4;
bool StateLED = false;

//Variables pour spécifier la couleur de la LED Strip
int R = 200, G = 100, B = 10;
/**
 * @brief callback function for MQTT message from OpenHAB
 * 
 * @param topic topic that the message came from
 * @param payload message sent from OpenHAB
 * @param length size of message
 */
void callback(char* topic, byte* payload, unsigned int length);
/**
 * @brief Function who help to connect to the mqtt broker
 * 
 */
void MQTTConnect();
/**
 * @brief function who set the RGB color to the Strip LED
 * 
 * @param c probobly the result of pixel.color(100,200,255) function
 */
void colorWipe(uint32_t c);

Adafruit_NeoPixel RGB_Strip = Adafruit_NeoPixel(NUM_LED, PIN_LED, NEO_GRB + NEO_KHZ800);

/**
 * @brief Set the up wifi object
 * 
 */
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  Serial.print("Topic : ");
  Serial.println(topic);
  Serial.print("Payload : ");

  for (int i = 0; i < length; i++) {
    //Create our String variable who contains mqtt message
    message += (char)payload[i];
  }
  Serial.println(message);
  if(!strcmp(chaine1, topic)){
    //Serial.println("Chaine 1");
    sscanf(message.c_str(), "%d", &MotorSpeed);
    analogWrite(PinMoteur, MotorSpeed);
  }
  if(!strcmp(chaine2, topic)){
    //Serial.println("Chaine 2");
    sscanf(message.c_str(), "%d,%d,%d", &R, &G, &B);
    //Serial.println(message.c_str());
    //Serial.println(B);
    colorWipe(RGB_Strip.Color(R, G, B));

  }
  if(!strcmp(chaine3, topic)){
    //Serial.println("Chaine 3");
    if(message == "ON")
      StateLED = true;
    else if(message == "OFF")
      StateLED = false;
    digitalWrite(PinLED, StateLED);

  }
}

void MQTTConnect(){
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("Connexion au broker MQTT...");
  //Connexion au broker MQTT
  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password )) {
      Serial.println("Connecté au broker MQTT");
    } else {
      Serial.print("Échec de connexion au broker MQTT, code erreur = ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  // Subscribing to the topics
  client.subscribe(chaine1);
  client.subscribe(chaine2);
  client.subscribe(chaine3);
  //client.subscribe(chaine4);
  Temps.startTimer(wait);
}

void setup() {
  Serial.begin(9600);
  // Configuration des broches.
  pinMode(PinBouton, INPUT);
  pinMode(PinMoteur, OUTPUT);
  pinMode(PinLED, OUTPUT);
  pinMode(PinRotation, INPUT);

  // Configuration du réseau
  setup_wifi();
  MQTTConnect();
  // Setup the first color for the led
  RGB_Strip.begin();
  RGB_Strip.show();
  // Je voulais configurer le bouton pour activer et désactiver la LED, mais ça n'a pas marché.
  // J'ignore la raison pour le moment.
  RGB_Strip.setBrightness(255);    // Set brightness, 0-255 (darkest - brightest)
  colorWipe(RGB_Strip.Color(R, G, B));
}

void loop() {
  client.loop();
  int sensorValue = analogRead(PinRotation);
  //Si le bouton est préssé, envoyer l'état sur OpenHAB
  if (digitalRead(PinBouton))
  {
    StateLED = !StateLED;
    if(StateLED){
      client.publish("capteur/digital", "ON");
    }
    else{
      client.publish("capteur/digital", "OFF");
    }
    // Attendre un peu afin d'éviter les actions trop rapides
    delay(200);
  }

  // juste une autre façon plus classe d'utiliser la fonction millis() :)
  if(Temps.isTimerReady()){
    // Conversion d'un entier en chaine de caractères (char*)
    char nomb[4];
    client.publish(chaine4, itoa(sensorValue, nomb, 10));
    Temps.startTimer(wait);
  }
  digitalWrite(PinLED, StateLED);
}

void colorWipe(uint32_t c) {
  for (uint16_t i = 0; i < RGB_Strip.numPixels(); i++) {
    RGB_Strip.setPixelColor(i, c);
    RGB_Strip.show();
  }
}