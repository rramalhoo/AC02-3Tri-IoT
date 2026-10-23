#include <SPI.h>           // comunicação SPI 
#include <MFRC522.h>       // leitor RFID RC522
#include <ESP32Servo.h>    // servo motor
#include <WiFi.h>          // conexão Wi-Fi
#include <PubSubClient.h>  // MQTT

// ---------- LEITOR RFID ----------
// Pinos: SDA no 5 e RST no 21
MFRC522 rfid(5, 21);

// ---------- CARTÕES AUTORIZADOS ----------
String autorizados[] = {
  "01:02:03:04",
  "11:22:33:44"
};
int totalAutorizados = 2;

// ---------- FECHADURA, WI-FI E MQTT ----------
Servo fechadura;                 // servo no pino 13
WiFiClient wifiClient;           // conexão de rede
PubSubClient mqtt(wifiClient);   // cliente MQTT usando essa conexão

// Tópico onde as mensagens são publicadas
const char* TOPICO = "armario/ramalho/A01";

// ---------- CONTADOR ----------
int leituras = 0;   // quantos cartões já foram lidos

// ---------- FUNÇÃO QUE MONTA E PUBLICA O JSON ----------
void publicarEvento(String evento, String uid, String tipoCartao) {
  // Monta o JSON juntando os pedaços
  String json = "{";
  json += "\"armario\":\"A01\",";
  json += "\"evento\":\"" + evento + "\",";
  json += "\"uid\":\"" + uid + "\",";
  json += "\"tipo_cartao\":\"" + tipoCartao + "\",";
  json += "\"leituras\":" + String(leituras);
  json += "}";

  Serial.println(json);                  // mostra no Serial Monitor
  mqtt.publish(TOPICO, json.c_str());    // envia para o broker
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);   // liga o Serial Monitor
  SPI.begin();            // liga a comunicação SPI
  rfid.PCD_Init();        // liga o leitor RFID

  // LEDs: 26 = verde, 27 = vermelho
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);

  // Fechadura começa com 0 graus e com o LED vermelho aceso
  fechadura.attach(13);
  fechadura.write(0);
  digitalWrite(27, HIGH);

  // Conecta na rede
  Serial.print("Conectando na rede");
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println(" conectado!");

  // Configura o broker MQTT
  mqtt.setServer("broker.hivemq.com", 1883);
  mqtt.setBufferSize(512);

  Serial.println("Leitor Pronto!");
}


// ---------- LOOP ----------
void loop() {
  // broker
  if (!mqtt.connected()) {
    Serial.println("Conectando no broker...");
    if (mqtt.connect("armario-ramalho-A01")) {   // nome
      Serial.println("Broker conectado!");
    } else {
      Serial.println("Falhou, tentando de novo");
      delay(2000);
      return;
    }
  }
  mqtt.loop();   // mantém a conexão MQTT

  // Espera um cartão
  if (!rfid.PICC_IsNewCardPresent()) return;   // tem cartão perto?
  if (!rfid.PICC_ReadCardSerial()) return;     // conseguiu ler?

  // Transforma o UID para o formato "01:02:03:04"
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";      // zero à esquerda
    uid += String(rfid.uid.uidByte[i], HEX);         // byte em hexadecimal
    if (i < rfid.uid.size - 1) uid += ":";           // dois pontos entre os bytes
  }
  uid.toUpperCase();
  Serial.println("UID: " + uid);

  // Conta a leitura e descobre o tipo do cartão
  leituras++;
  MFRC522::PICC_Type tipo = MFRC522::PICC_GetType(rfid.uid.sak);
  String tipoCartao = MFRC522::PICC_GetTypeName(tipo);

  // Procura o UID na lista de autorizados
  bool liberado = false;
  for (int i = 0; i < totalAutorizados; i++) {
    if (uid == autorizados[i]) {
      liberado = true;
    }
  }

  // Decide o que fazer
  if (liberado) {
    Serial.println("ACESSO LIBERADO");
    publicarEvento("ACESSO_LIBERADO", uid, tipoCartao);

    // Abre: servo a 90 graus, LED verde aceso
    fechadura.write(90);
    digitalWrite(26, HIGH);
    digitalWrite(27, LOW);

    delay(5000);   // fica aberto por 5 segundos

    // Tranca de novo: servo a 0 graus, LED vermelho aceso
    fechadura.write(0);
    digitalWrite(26, LOW);
    digitalWrite(27, HIGH);
    Serial.println("ARMARIO TRANCADO");
  } else {
    Serial.println("ACESSO NEGADO");
    publicarEvento("ACESSO_NEGADO", uid, tipoCartao);
  }

  // Desabilita o cartão para não ser lido várias vezes
  rfid.PICC_HaltA();
}