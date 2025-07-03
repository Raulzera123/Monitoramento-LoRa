import serial
import json
import paho.mqtt.client as mqtt
import time
import sys

# --- Configurações ---
# Porta serial à qual o módulo LoRa está conectado
# No Raspberry Pi 4, geralmente é '/dev/ttyS0' ou '/dev/serial0'
SERIAL_PORT = '/dev/ttyS0' 
BAUD_RATE = 9600  # Deve ser a mesma configurada no seu módulo LoRa/Arduino

# Configurações do Broker MQTT (usando o broker público da HiveMQ)
MQTT_BROKER = 'broker.hivemq.com'
MQTT_PORT = 1883
MQTT_BASE_TOPIC = 'lora/sensores' # Tópico base para publicação

# --- Fim das Configurações ---


# Função chamada quando o cliente se conecta ao broker MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado ao Broker MQTT com sucesso!")
    else:
        print(f"Falha ao conectar, código de retorno: {rc}\n")

# Função para publicar os dados do sensor
def publish_sensor_data(client, sensor_data):
    try:
        # Extrai os dados do JSON
        sensor_id = sensor_data.get('id')
        distancia = sensor_data.get('d')
        temperatura = sensor_data.get('t')

        # Verifica se todos os dados necessários estão presentes
        if sensor_id and distancia is not None and temperatura is not None:
            # Monta os tópicos MQTT dinamicamente
            topic_dist = f"{MQTT_BASE_TOPIC}/{sensor_id}/distancia"
            topic_temp = f"{MQTT_BASE_TOPIC}/{sensor_id}/temperatura"

            # Publica os dados nos respectivos tópicos
            client.publish(topic_dist, payload=distancia, qos=1, retain=True)
            client.publish(topic_temp, payload=temperatura, qos=1, retain=True)

            print(f"  > Publicado em '{topic_dist}': {distancia}")
            print(f"  > Publicado em '{topic_temp}': {temperatura}")
        else:
            print("(!) JSON recebido está incompleto. Ignorando.")

    except Exception as e:
        print(f"(!) Erro ao processar ou publicar os dados: {e}")


# --- Programa Principal ---
if __name__ == '__main__':
    # Inicializa o cliente MQTT
    client = mqtt.Client(client_id=f"raspberry-pi-gateway-{int(time.time())}")
    client.on_connect = on_connect
    
    print(f"Conectando ao broker MQTT em {MQTT_BROKER}...")
    try:
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
    except Exception as e:
        print(f"Não foi possível conectar ao MQTT Broker. Verifique a conexão com a internet.")
        print(f"Erro: {e}")
        sys.exit(1)

    # Inicia o loop do cliente MQTT em uma thread separada para não bloquear o script
    client.loop_start()

    # Tenta abrir a porta serial
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        # Limpa qualquer dado residual no buffer da serial
        ser.flushInput()
        print(f"Escutando na porta serial {SERIAL_PORT} a {BAUD_RATE} bps...")
    except serial.SerialException as e:
        print(f"(!) Erro ao abrir a porta serial {SERIAL_PORT}: {e}")
        print("Verifique se a porta está correta e se você tem permissão para acessá-la (use 'sudo').")
        client.loop_stop()
        sys.exit(1)

    try:
        while True:
            # Lê uma linha da porta serial
            if ser.in_waiting > 0:
                line = ser.readline()
                
                # Tenta decodificar a linha (vem como bytes) e remove espaços em branco
                try:
                    message = line.decode('utf-8').strip()
                except UnicodeDecodeError:
                    print("(!) Recebido dado não-UTF8, ignorando.")
                    continue

                if message:
                    print(f"\nRecebido via LoRa: {message}")
                    
                    # Tenta converter a string recebida em um objeto JSON
                    try:
                        data = json.loads(message)
                        publish_sensor_data(client, data)
                    except json.JSONDecodeError:
                        print("(!) Erro: A mensagem recebida não é um JSON válido.")

            # Pequena pausa para não sobrecarregar a CPU
            time.sleep(0.1)

    except KeyboardInterrupt:
        print("\nPrograma interrompido pelo usuário. Desconectando...")
    finally:
        # Garante que a porta serial e a conexão MQTT sejam fechadas
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Porta serial fechada.")
        client.loop_stop()
        client.disconnect()
        print("Cliente MQTT desconectado.")
