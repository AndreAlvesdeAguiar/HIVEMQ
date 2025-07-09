# Ajustes no `lwipopts.h` para Estabilidade e Performance (Pico W + MQTT)

Este projeto utiliza o Raspberry Pi Pico W com a pilha lwIP para conexão Wi-Fi e publicação de dados via MQTT. Durante testes iniciais, a aplicação apresentava instabilidade e falhas com a seguinte mensagem de erro:

*** PANIC ***
pcb->snd_queuelen >= pbuf_clen(next->p)


Após investigação, foram realizadas modificações críticas no arquivo `lwipopts.h` para resolver esse problema e garantir estabilidade mesmo com publicações periódicas de dados.

---

## 🔧 Alterações Realizadas

| Parâmetro | Antes | Depois | Descrição |
|----------|-------|--------|-----------|
| `SYS_LIGHTWEIGHT_PROT` | — | `1` | Habilita proteção leve para estruturas críticas da pilha TCP. Essencial com `NO_SYS=1` e modo `threadsafe_background`. |
| `MEM_SIZE` | `4000` | `8192` | Aumenta o heap interno do lwIP. Necessário para múltiplos pacotes MQTT e conexões estáveis. |
| `MEMP_NUM_TCP_SEG` | `32` | `64` | Duplica a capacidade de segmentos TCP simultâneos. Reduz falhas em transmissões seguidas. |
| `PBUF_POOL_SIZE` | `24` | `64` | Aumenta a quantidade de buffers de pacotes (`pbuf`). Evita falta de memória durante envio ou recepção. |
| `MEM_DEBUG` | `OFF` | `ON` | Habilita logs de debug da alocação de memória. Útil para rastrear vazamentos e erros. |
| `MEMP_DEBUG` | `OFF` | `ON` | Habilita debug específico para os pools de memória (`memp`). |
| `TCP_DEBUG` | `OFF` | `ON` | Exibe mensagens do funcionamento da pilha TCP (envio, filas, retransmissões). |
| `TCP_OUTPUT_DEBUG` | `OFF` | `ON` | Ajuda a rastrear o comportamento do `tcp_write()` e `tcp_output()`. |

---

## ✅ Resultado

Após essas modificações, a aplicação:

- Se conecta ao Wi-Fi e ao broker MQTT de forma confiável.
- Publica dados de temperatura e botão a cada 1 segundo, com estabilidade contínua.
- Não apresenta mais erros de assert relacionados ao `pcb->snd_queuelen`.
- Ganha visibilidade para depuração via logs (`DEBUG`).

---

## 💡 Recomendação

Essas alterações devem ser consideradas **padrão mínimo** para qualquer projeto que utilize:

- `NO_SYS = 1`
- `pico_cyw43_arch_lwip_threadsafe_background`
- Publicação frequente via TCP (MQTT, HTTP, etc.)
- Uso de buffers intensivo (sensores, streaming, logs)

---

## 📂 Arquivo Modificado

O arquivo alterado é:

pico-sdk/src/boards/your_board_name/lwipopts.h


Verifique se ele está sendo usado na compilação e não sobrescrito por configurações locais de biblioteca.

---


# 🛰️ Instalação e Uso do Broker Mosquitto no Windows

Este guia explica como instalar e rodar o **broker MQTT Mosquitto** em uma máquina com Windows, permitindo o envio e recebimento de mensagens MQTT com dispositivos IoT, como a **BitDog Lab**.

---

## 📥 1. Baixar o Mosquitto

1. Acesse: [https://mosquitto.org/download](https://mosquitto.org/download)
2. Baixe o instalador para Windows (.exe)
3. Durante a instalação:
   - Marque a opção para instalar como **serviço**
   - Finalize normalmente

---

## 🔥 2. Liberar o Mosquitto no Firewall

No terminal como **Administrador**, execute:

```bash
netsh advfirewall firewall add rule name="Mosquitto MQTT" dir=in action=allow protocol=TCP localport=1883
```

---

## 🗂️ 3. Criar o arquivo de configuração

Por padrão, o Mosquitto aceita apenas conexões locais. Para permitir conexões externas:

### Etapas:

1. Crie a pasta:

```
C:\Program Files\mosquitto\mosquitto_config
```

2. Crie um arquivo `mosquitto.conf` dentro dessa pasta com o seguinte conteúdo:

```conf
listener 1883
allow_anonymous true
```

---

## ▶️ 4. Rodar o Mosquitto manualmente

Abra o terminal e execute:

```bash
cd "C:\Program Files\mosquitto"
"mosquitto.exe" -c "C:\Program Files\mosquitto\mosquitto_config\mosquitto.conf" -v
```

Você verá logs como:

```
mosquitto version 2.0.21 starting
Config loaded from C:\Program Files\mosquitto\mosquitto_config\mosquitto.conf
Opening ipv4 listen socket on port 1883.
```

---

## 📡 5. Testar comunicação MQTT (LED e botão)

### 🔘 Publicar o estado do botão (BitDog Lab → broker):

```c
mqtt_comm_publish("bitdoglab/entrada/botao1", (uint8_t *)"1", 1);
```

Para escutar esse tópico no PC:

```bash
mosquitto_sub -h localhost -t "bitdoglab/entrada/botao1"
```

---

### 💡 Comandar o LED via MQTT (PC → BitDog Lab):

Ligue o LED:

```bash
mosquitto_pub -h localhost -t "bitdoglab/saida/led1" -m "1"
```

Desligue o LED:

```bash
mosquitto_pub -h localhost -t "bitdoglab/saida/led1" -m "0"
```

> ✅ O dispositivo precisa estar **inscrito** no tópico `bitdoglab/saida/led1` para receber o comando.

---

## 🛠️ Ferramentas úteis

- [MQTT Explorer](https://mqtt-explorer.com/)
- `mosquitto_pub`, `mosquitto_sub` (linha de comando)
- [HiveMQ WebClient](https://www.hivemq.com/demos/websocket-client/) (teste via navegador)

---

## ✅ Pronto! Seu notebook agora é um broker MQTT

A BitDog Lab pode se comunicar diretamente com o PC para enviar e receber comandos via Wi-Fi.


### Complementares
📌 STACK (PILHA)
✔️ O que é?
Área da memória usada para armazenar variáveis locais e chamadas de função.

🔧 Como funciona?
A alocação é automática.

Cresce e diminui conforme o programa entra e sai de funções.

Rápida e organizada (tipo LIFO – último a entrar, primeiro a sair).

Tamanho limitado (ex: 2 KB, 4 KB em microcontroladores).

📌 HEAP
✔️ O que é?
Área da memória usada para alocação dinâmica (em tempo de execução).

🔧 Como funciona?
Você aloca manualmente com malloc() e libera com free().

Mais flexível, mas mais lenta que a stack.

Gerenciada por ponteiros.

*** PANIC ***
pcb->snd_queuelen >= pbuf_clen(next->p) está relacionado à HEAP, não à stack.

