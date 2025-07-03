#include <Arduino.h>
#include "LoRa_E32.h"

#define E32_RX 3 // RX Arduino -> TX do E32
#define E32_TX 4 // TX Arduino -> RX do E32
#define E32_AUX 2

LoRa_E32 e32ttl(E32_RX, E32_TX, E32_AUX);

void setup() {
  Serial.begin(9600);
  while(!Serial){;}
  
  e32ttl.begin();
  delay(100);

  Serial.println("Cliente (UNO) iniciado.");
  delay(500);
}

void loop() {
  Serial.println("Enviando requisicao...");
  // Mandando para endereço (0,5), canal 4 (módulo configurado c/ address=5, channel=4)
  ResponseStatus rs = e32ttl.sendFixedMessage(0, 5, 0x04, "REQ");
  if (rs.code != 1) {
    Serial.print("Erro ao enviar REQ: ");
    Serial.println(rs.getResponseDescription());
  }

  // Aguarda resposta (até X ms)
  String jsonData = "";
  unsigned long startWait = millis();
  bool gotResponse = false;
  while (millis() - startWait < 5000) {
    if (e32ttl.available() > 0) {
      ResponseContainer rc = e32ttl.receiveMessage();
      if (rc.status.code == 1) {
        jsonData = rc.data;
        Serial.print("Resposta recebida (JSON): ");
        Serial.println(jsonData);
        gotResponse = true;
        break;
      } else {
        Serial.print("Falha ao receber resposta: ");
        Serial.println(rc.status.getResponseDescription());
      }
    }
  }

  if (!gotResponse) {
    Serial.println("Nenhuma resposta recebida dentro do tempo limite.");
  } else {
    // Exemplo: encaminhar ao Raspberry Pi 4 (endereço 0,1,0x04 ?)
    // Ajuste se seu RPi tiver outro endereço/config
    ResponseStatus rs2 = e32ttl.sendFixedMessage(0, 1, 0x04, jsonData);
    if (rs2.code == 1) {
      Serial.println("JSON encaminhado ao Raspberry Pi 4 com sucesso.");
    } else {
      Serial.print("Falha ao enviar JSON ao Raspberry Pi: ");
      Serial.println(rs2.getResponseDescription());
    }
  }

  // Aguarda alguns segundos antes de nova requisição
  delay(5000);
}
