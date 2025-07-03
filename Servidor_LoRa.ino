#include <Arduino.h>
#include "LoRa_E32.h"
#include "HCSR04.h"
#include <Wire.h>
#include "Adafruit_MLX90614.h"
#include "LowPower.h"        // Biblioteca para o modo de baixo consumo
#include <SoftwareSerial.h>  // Necessária para a comunicação com o LoRa E32

// Definindo o nome do sensor
#define SENSOR_ID "A1"

// --- Pinos do LoRa E32 ---
// É necessário definir M0 e M1 para a biblioteca, mesmo que os aterre
#define E32_M0_PIN 4
#define E32_M1_PIN 5
#define E32_RX_PIN 11 // Conectado ao pino TX do E32
#define E32_TX_PIN 12 // Conectado ao pino RX do E32
#define E32_AUX_PIN 2 // Pino para acordar o Arduino (precisa ser um pino de interrupção)

// --- Pinos do HC-SR04 ---
#define TRIG_PIN 9
#define ECHO_PIN 8

// Inicializa os sensores e o LoRa
LoRa_E32 e32ttl(E32_TX_PIN, E32_RX_PIN, E32_AUX_PIN);
UltraSonicDistanceSensor hcsr04(TRIG_PIN, ECHO_PIN);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();


// Função que será chamada pela interrupção para acordar o microcontrolador
// Pode ficar vazia, seu único propósito é acordar o chip.
void wakeUp() {
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!mlx.begin()) {
    Serial.println("Falha ao iniciar MLX90614");
    while (1); // Trava se o sensor falhar
  }

  e32ttl.begin();
  delay(500);

  // Configura o pino da interrupção para acordar o Arduino
  pinMode(E32_AUX_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(E32_AUX_PIN), wakeUp, LOW);

  Serial.println("Setup concluído. Entrando em modo de hibernação...");
  Serial.flush(); // Garante que a mensagem serial seja enviada antes de hibernar
  
  // Coloca o Arduino para hibernar indefinidamente até uma interrupção ocorrer
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void loop() {
  
  // Aguarda um pequeno instante para garantir que os dados estão prontos no buffer
  delay(50);

  if (e32ttl.available() > 1) {
    // O Arduino acorda aqui quando uma mensagem chega (o pino AUX fica em LOW)
    Serial.println("Arduino acordou!");
    ResponseContainer rc = e32ttl.receiveMessage();
    if (rc.status.code == 1) {
      String msg = rc.data;
      Serial.print("Mensagem recebida: ");
      Serial.println(msg);

      // --- Medição dos sensores ---
      float distancia = hcsr04.measureDistanceCm();
      float temperatura = mlx.readObjectTempC();
      
      // --- Montagem da resposta JSON ---
      String json = "{\"id\":\"";
      json += String(SENSOR_ID);
      json += "\",\"d\":";
      json += String(distancia, 2);
      json += ",\"t\":";
      json += String(temperatura, 2);
      json += "}";

      // Envia a resposta de volta (ex: para o nó de endereço 0, canal 12, com ADDH=0 e ADDL=0x04)
      ResponseStatus rs = e32ttl.sendFixedMessage(0, 12, 0x04, json);
      if (rs.code == 1) {
        Serial.print("Resposta enviada: ");
        Serial.println(json);
      } else {
        Serial.print("Falha ao enviar resposta: ");
        Serial.println(rs.getResponseDescription());
      }

    } else {
      Serial.print("Falha ao receber: ");
      Serial.println(rc.status.getResponseDescription());
    }
  }

  // Após processar, volta a hibernar
  Serial.println("Processamento concluído. Voltando a hibernar...");
  Serial.flush();
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}
