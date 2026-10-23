============================================================
 ARMÁRIO INTELIGENTE COM RFID E MQTT
 AC02 - IoT - 3º trimestre
============================================================
Integrantes: Ana Elisa e Raphael Ramalho
Link do projeto no Wokwi: https://wokwi.com/projects/478502305775727617

------------------------------------------------------------
1. O QUE O PROJETO FAZ
------------------------------------------------------------
Um **ESP32** com leitor** RFID RC522** controla um armário. Ao aproximar
um cartão, o ESP32 lê o identificador (UID) e confere se ele está
na lista de autorizados:
- Autorizado: o servo (fechadura) gira para 90°, o LED verde
  acende e, depois de 5 s, o armário tranca sozinho.
- Não autorizado: o armário continua trancado (LED vermelho).
Nos dois casos, o ESP32 publica uma mensagem JSON no broker MQTT.

------------------------------------------------------------
2. ARQUIVOS
------------------------------------------------------------
skesketch.ino   -> código do ESP32 (comentado)
diadiagram.json   -> circuito do Wokwi
liblibraries.txt  -> bibliotecas do projeto
Relatório em PDF

------------------------------------------------------------
3. BIBLIOTECAS
------------------------------------------------------------
- MFRC522       -> leitor RFID RC522 (miguelbalboa/rfid)
- ESP32Servo    -> servo motor (fechadura)
- PubSubClient  -> cliente MQTT
- SPI.h e WiFi.h já vêm com o ESP32.
No Wokwi elas são instaladas automaticamente pelo libraries.txt.

------------------------------------------------------------
4. CIRCUITO
------------------------------------------------------------
Leitor RFID RC522: SDA=5, SCK=18, MOSI=23, MISO=19, RST=21, 3.3V=3V3, GND=GND

  **Servo** (fechadura): PWM=13, V+=5V, GND=GND
  **LED verde** (aberto): pino 26, com resistor de 220 ohms
  **LED vermelho** (trancado): pino 27, com resistor de 220 ohms

------------------------------------------------------------
5. BROKER E TÓPICO
------------------------------------------------------------
Wi-Fi:   Wokwi-GUEST (rede do Wokwi)
Broker:  broker.hivemq.com
Porta:   1883
Tópico:  armario/ramalho/A01

------------------------------------------------------------
6. COMO EXECUTAR (ordem de inicialização)
------------------------------------------------------------
1) Abra o HiveMQ Web Client:
   https://www.hivemq.com/demos/websocket-client/
2) Clique em Connect (Host broker.hivemq.com, Port 8884, SSL
   marcado). Deve aparecer "connected".
3) Clique em "Add New Topic Subscription", digite
   armario/ramalho/A01 (ou armario/#) e clique em Subscribe.
4) Abra o link do projeto no Wokwi e clique em Play.
5) Espere aparecer no Serial Monitor, nesta ordem:
     Conectando no Wi-Fi... conectado!
     Leitor Pronto!
     Conectando no broker...
     Broker conectado!
6) Clique no leitor RFID, escolha um cartão e clique em TAP.
7) Veja o JSON aparecer em "Messages" no HiveMQ.

------------------------------------------------------------
7. QUANDO A MENSAGEM É PUBLICADA
------------------------------------------------------------
Uma mensagem é publicada a cada cartão lido.

Cartões autorizados -> evento ACESSO_LIBERADO
  **Blue Card**  (01:02:03:04)
  **Green Card **(11:22:33:44)

Qualquer outro cartão -> evento ACESSO_NEGADO
  **Yellow Card**, **Red Card**,** NFC Tag, Key Fob**

O mesmo cartão parado no leitor com a opcao **HOLD** ativa gera apenas UMA
mensagem, pois o cartão é colocado em espera (PICC_HaltA).

Exemplos:
{"armario":"A01","evento":"ACESSO_LIBERADO","uid":"01:02:03:04","tipo_cartao":"MIFARE 1KB","leituras":1}
{"armario":"A01","evento":"ACESSO_NEGADO","uid":"AA:BB:CC:DD","tipo_cartao":"MIFARE 1KB","leituras":2}

------------------------------------------------------------
8. MELHORIA DO GRUPO
------------------------------------------------------------
O exemplo da catraca visto em aula apertava um botão, liberava
o servo e enviava apenas uma mensagem simples de acesso.

Na nossa versão, o ESP32 passa a:
- identificar o TIPO do cartão pelo byte SAK ;
- montar o UID com qualquer número de bytes;
- contar as leituras;
- publicar um JSON com 4 informações: identificador (uid), tipo
  do cartão (tipo_cartao), tipo de evento (evento) e contador de
  leituras (leituras).

Trechos responsáveis no sketch.ino:
- função publicarEvento() -> monta e publica o JSON;
- passo 4 do loop() -> leituras++, PICC_GetType() e
  PICC_GetTypeName().

Limitação conhecida: no teste com a NFC Tag, o UID de 7 bytes
chegou com apenas 4 bytes (04:11:22:33). O tipo do cartão foi
identificado corretamente.
