# Sistema de Monitoramento Inteligente de Resíduos Urbanos
**Universidade Presbiteriana Mackenzie**  
Autoras: Bianca Maciel Alaunes Brotto, Seyedehzahra Mousavi  


---

## Descrição

Sistema embarcado para monitoramento do nível de preenchimento de lixeiras urbanas utilizando IoT. O ESP32 lê periodicamente a distância até os resíduos via sensor ultrassônico HC-SR04, calcula o percentual de preenchimento, aciona um buzzer passivo como alerta sonoro local quando o nível ultrapassa 80%, e publica os dados em tempo real via protocolo MQTT para monitoramento remoto.

---

## Como reproduzir

### ⚠️ Importante: dois ambientes de simulação

Este projeto utiliza dois ambientes distintos:

| Ambiente | Uso | API LEDC |
|---|---|---|
| **Wokwi VS Code** (recomendado) | Simulação com MQTT real | `ledcSetup` / `ledcAttachPin` (core v2) |
| **Wokwi Online** (referência visual) | Visualização do circuito | `ledcAttach` / `ledcDetach` (core v3+) |

O código em `src/sketch.ino` é compatível com o **Wokwi VS Code via PlatformIO**, que utiliza o ESP32 Arduino Core v2. O link do Wokwi online abaixo serve apenas para visualização do diagrama do circuito.

### Simulação com Wokwi + VS Code (recomendado)

1. Instale o [Visual Studio Code](https://code.visualstudio.com/)
2. Instale as extensões **PlatformIO IDE** e **Wokwi Simulator**
3. Clone ou baixe este repositório
4. Abra a pasta no VS Code (**File → Open Folder**)
5. Abra o terminal PlatformIO e compile: `pio run`
6. Inicie a simulação: **Ctrl+Shift+P → Wokwi: Start Simulator**
7. O ESP32 conectará ao Wi-Fi (Wokwi-GUEST) e ao broker MQTT automaticamente

### Visualização do circuito (Wokwi Online)

Acesse o diagrama do circuito em:  
https://wokwi.com/projects/465559332045602817

> ⚠️ O código do repositório **não é compatível** com o Wokwi online diretamente, pois utilizam versões diferentes do ESP32 Arduino Core. Use o ambiente VS Code para executar o projeto completo.

### Monitorar mensagens MQTT

1. Acesse: https://www.hivemq.com/demos/websocket-client/
2. Conecte em `broker.hivemq.com` porta `8884`
3. Assine o tópico `lixeira/L01/nivel`
4. As mensagens aparecem automaticamente a cada 2 segundos

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
| HC-SR04 | VCC | 5V |
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
├── diagram.json          # Diagrama de montagem do circuito (Wokwi)
├── platformio.ini        # Configuração do PlatformIO
├── wokwi.toml            # Configuração do simulador Wokwi
├── libraries.txt         # Bibliotecas necessárias
└── README.md
```

### Bibliotecas necessárias
- `WiFi.h` — nativa do ESP32
- `PubSubClient` v2.8+ — comunicação MQTT
- `ArduinoJson` v6+ — serialização JSON

### Lógica principal
1. Conecta ao Wi-Fi e ao broker MQTT (não-bloqueante)
2. A cada 2 segundos, lê a distância pelo HC-SR04
3. Calcula o nível: `Nível(%) = ((dist_vazia - dist_medida) / (dist_vazia - dist_cheia)) × 100`
4. Se nível ≥ 80%: aciona o buzzer passivo (PWM 2kHz)
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
  "distancia_cm": 15,
  "alerta": false
}
```

---

## Referências
- ESPRESSIF SYSTEMS. ESP32 Datasheet. 2023.
- ELECFREAKS. HC-SR04 Datasheet. 2011.
- HANDSONTEC. Passive Buzzer Module. 2015.
- YUAN, M. Why MQTT is one of the best network protocols for IoT. IBM Developer, 2021.
- MQTT.ORG. MQTT: the standard for IoT messaging. 2019.
