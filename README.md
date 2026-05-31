# Sistema de Monitoramento Inteligente de Resíduos Urbanos
**Universidade Presbiteriana Mackenzie — Faculdade de Computação e Informática**  
Autoras: Bianca Maciel Alaunes Brotto, Seyedehzahra Mousavi  
Disciplina: Internet das Coisas (IoT)

---

## Descrição

Sistema embarcado para monitoramento do nível de preenchimento de lixeiras urbanas utilizando IoT. O ESP32 lê periodicamente a distância até os resíduos via sensor ultrassônico HC-SR04, calcula o percentual de preenchimento, aciona um buzzer passivo como alerta sonoro local quando o nível ultrapassa 80%, e publica os dados em tempo real via protocolo MQTT para monitoramento remoto.

---

## Como reproduzir

### Simulação online (Wokwi)
1. Acesse o projeto simulado: https://wokwi.com/projects/465559332045602817
2. Clique em ▶️ Play para iniciar a simulação
3. O ESP32 irá conectar ao Wi-Fi (Wokwi-GUEST) e ao broker MQTT automaticamente

### Hardware físico
1. Monte o circuito conforme a seção de hardware abaixo
2. Abra o arquivo `src/sketch.ino` na Arduino IDE
3. Instale as bibliotecas: `PubSubClient` e `ArduinoJson`
4. Configure suas credenciais Wi-Fi nas linhas 24–25
5. Faça o upload para o ESP32

---

## Hardware

### Componentes
| Componente | Quantidade | Função |
|---|---|---|
| ESP32 DevKit v1 | 1 | Microcontrolador principal, Wi-Fi e MQTT |
| Sensor HC-SR04 | 1 | Medição ultrassônica do nível de preenchimento |
| Buzzer Passivo | 1 | Alerta sonoro local (nível > 80%) |
| Resistor 100 Ω | 1 | Proteção do GPIO do ESP32 |
| Protoboard 400 pts | 1 | Montagem dos componentes |
| Jumpers macho-macho | 10 | Conexões elétricas |
| Cabo USB Micro-B | 1 | Alimentação e programação |

### Mapeamento de pinos
| Componente | Pino do Componente | Pino ESP32 |
|---|---|---|
| HC-SR04 | VCC | VIN (5V) |
| HC-SR04 | GND | GND |
| HC-SR04 | TRIG | GPIO 5 |
| HC-SR04 | ECHO | GPIO 18 |
| Buzzer Passivo | + (positivo) | GPIO 23 (via resistor 100Ω) |
| Buzzer Passivo | - (negativo) | GND |

---

## Software

### Estrutura do repositório
```
smart-waste-monitoring-esp32/
├── src/
│   └── sketch.ino        # Firmware principal do ESP32
├── docs/
│   └── diagrama.png      # Diagrama de montagem do circuito
├── README.md
└── libraries.txt         # Bibliotecas necessárias
```

### Bibliotecas necessárias
- `WiFi.h` — nativa do ESP32
- `PubSubClient` v2.8+ — comunicação MQTT
- `ArduinoJson` v6+ — serialização JSON

### Lógica principal
1. Conecta ao Wi-Fi e ao broker MQTT
2. A cada 10 segundos, lê a distância pelo HC-SR04
3. Calcula o nível de preenchimento: `Nível(%) = ((dist_vazia - dist_medida) / (dist_vazia - dist_cheia)) × 100`
4. Se nível ≥ 80%: aciona o buzzer passivo (PWM 2kHz, intermitente)
5. Publica JSON no tópico MQTT `lixeira/L01/nivel`
6. Aguarda comandos remotos no tópico `lixeira/L01/cmd`

---

## Comunicação MQTT

### Broker
- **Endereço:** `broker.hivemq.com`
- **Porta:** `1883` (TCP) / `8884` (WebSocket SSL)
- **Protocolo:** MQTT 3.1.1

### Tópicos
| Tópico | Direção | Descrição |
|---|---|---|
| `lixeira/L01/nivel` | Publish | Dados do sensor em JSON |
| `lixeira/L01/cmd` | Subscribe | Comandos remotos (`beep_on`, `beep_off`) |

### Formato da mensagem publicada
```json
{
  "lixeira_id": "L01",
  "nivel_pct": 53,
  "distancia_cm": 15.0,
  "alerta": false
}
```

### Monitorar mensagens
Acesse o cliente WebSocket do HiveMQ:  
https://www.hivemq.com/demos/websocket-client/  
Conecte em `broker.hivemq.com:8884` e assine o tópico `lixeira/L01/nivel`

---

## Referências
- ESPRESSIF SYSTEMS. ESP32 Datasheet. 2023.
- ELECFREAKS. HC-SR04 Datasheet. 2011.
- HANDSONTEC. Passive Buzzer Module. 2015.
- YUAN, M. Why MQTT is one of the best network protocols for IoT. IBM Developer, 2021.
- MQTT.ORG. MQTT: the standard for IoT messaging. 2019.
