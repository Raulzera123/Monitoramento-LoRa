#include <Arduino.h>
#include "LoRa_E32.h"
#include "LowPower.h"        // Biblioteca para o modo de hibernação
#include <SoftwareSerial.h>  // Para comunicação nos pinos 3 e 4

// --- Pinos do LoRa E32 ---
#define E32_RX_PIN 3 // RX Arduino -> TX do E32
#define E32_TX_PIN 4 // TX Arduino -> RX do E32
#define E32_AUX_PIN 2 // Pino de interrupção para acordar o Arduino

// A biblioteca precisa dos pinos M0 e M1, mesmo que não os controle.
// Se não estiverem conectados, a biblioteca os ignora.
LoRa_E32 e32ttl(E32_TX_PIN, E32_RX_PIN, E32_AUX_PIN);

// Função que a interrupção chama para acordar o Arduino.
// Ela pode ficar vazia, seu propósito é apenas tirar o chip do modo sleep.
void wakeUp() {
  // A mágica acontece aqui, mas não precisamos de código.
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {;}

  e32ttl.begin();
  delay(500); // Aguarda a inicialização do módulo

  // Configura o pino AUX como entrada com pull-up e anexa a interrupção.
  // O Arduino acordará quando o pino AUX for para o nível BAIXO (LOW).
  pinMode(E32_AUX_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(E32_AUX_PIN), wakeUp, LOW);

  Serial.println("Modo repetidor iniciado. Entrando em modo de hibernação...");
  Serial.flush(); // Garante que a mensagem seja enviada antes de dormir

  // Coloca o Arduino para dormir até que a interrupção ocorra
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void loop() {
  // O código continua daqui quando o Arduino acorda
  Serial.println("\nArduino acordou! Verificando mensagem...");

  // Uma pequena pausa para garantir que o buffer serial do LoRa esteja pronto
  delay(50);

  // Verifica se há dados disponíveis no módulo LoRa
  if (e32ttl.available() > 0) {
    ResponseContainer rc = e32ttl.receiveMessage();

    if (rc.status.code == 1) {
      String jsonData = rc.data;
      Serial.print("Pacote recebido: ");
      Serial.println(jsonData);

      // Tenta encaminhar o pacote recebido para o Raspberry Pi
      // Endereço do RPi: 0, Canal: 5, ADDH=0, ADDL=0x04 (ajuste conforme necessário)
      Serial.println("Encaminhando pacote para o Gateway Raspberry Pi...");
      ResponseStatus rs = e32ttl.sendFixedMessage(0, 5, 0x04, jsonData);

      if (rs.code == 1) {
        Serial.println("Pacote encaminhado com sucesso.");
      } else {
        Serial.print("Falha ao encaminhar pacote: ");
        Serial.println(rs.getResponseDescription());
      }
    } else {
      Serial.print("Falha ao receber pacote: ");
      Serial.println(rc.status.getResponseDescription());
    }
  } else {
      Serial.println("Acordou, mas nenhum dado foi lido. Verifique a conexão.");
  }

  // Volta a dormir para economizar energia
  Serial.println("Processamento concluído. Voltando a hibernar...");
  Serial.flush();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
