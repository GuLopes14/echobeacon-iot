# Projeto EchoBeacon para Mottu 🏍️

# Integrantes
* **Gustavo Lopes Santos da Silva** - RM: 556859
* **Renato de Freitas David Campiteli** - RM: 555627
* **Gabriel Santos Jablonski** - RM: 555452

Nota: este projeto não utiliza mais backend local. O app mobile com Firebase está neste repositório: https://github.com/GuLopes14/echobeacon-mobile

## Resumo da solução 🚀

O projeto EchoBeacon visa implementar uma solução tecnológica para melhorar a organização e a localização das motos no pátio da empresa Mottu. O sistema será composto por uma série de componentes integrados, incluindo:

1. **EchoBeacon**: Dispositivos eletrônicos de tamanho reduzido instalados em cada moto para facilitar a identificação e o alarme.
2. **Sistema de Cadastro de Motos**: Desenvolvido com Java e NextJS para registrar informações detalhadas sobre as motos, incluindo chassi e problemas específicos.
3. **Aplicativo Móvel**: Conectado ao banco de dados centralizado para fornecer acesso aos colaboradores para consultar informações e localizar as motos.
4. **Banco de Dados Centralizado**: Armazena todas as informações relevantes sobre as motos, permitindo uma gestão eficiente.

### Funcionalidades do EchoBeacon ⚙️

- **Identificação Rápida**: Usando um sistema de som (pequeno alarme) e LED/farol.
- **Registro em Banco de Dados**: Quando uma moto chega ao pátio, suas informações são registradas para consulta posterior.

### Acesso aos Colaboradores

- **Consulta Detalhada**: Informações como placa, chassi e problema do veículo podem ser consultadas no aplicativo móvel.
- **Localização das Motos**: Funcionalidade para ativar o alarme e o LED da tag associada à moto selecionada, facilitando sua localização.

### Solução

Este sistema visa resolver o problema de localizar rapidamente as motos no pátio. Com a implementação desse sistema, a Mottu poderá organizar melhor suas motos e otimizar o tempo gasto na identificação e localização dos veículos dentro do pátio, garantindo uma gestão mais ágil e eficiente.

---

## Arquitetura Geral 🧩

Componentes e fluxo:
1. App mobile (botão "Localizar moto") publica comando MQTT no tópico: `fiap/iot/echobeacon/comando`.
2. Broker público HiveMQ retransmite a mensagem.
3. Beacons (ESP32 simulados no Wokwi) filtram pela placa e ativam LED + buzzer.

---

## Requisitos ✅
- Node.js 16+ instalado.
- npm disponível.
- Node-RED instalado (`npm install -g node-red`) ou via Docker (opcional).
- Navegador para abrir Wokwi (https://wokwi.com/).
- Acesso à internet (usa broker MQTT público).

---

## Passo a Passo de Execução (Ordem Recomendada) ▶️

### 1. Node-RED (Dashboard e Fluxo) 🖥️
1. Iniciar Node-RED:
```
node-red
```
2. Abrir: http://localhost:1880
3. Importar conteúdo de `node-red/flow.json` (Menu > Import > Paste > Deploy).
4. Abrir Dashboard: normalmente em `http://localhost:1880/ui`.
5. Verifique: painel/operação opcional para monitorar mensagens MQTT (sem botões de acionar; a publicação vem do app mobile).

### 2. Beacons no Wokwi (Simulação ESP32) 📡
Para cada pasta em `wokwi/echobeacon1`, `echobeacon2`, `echobeacon3`:
1. Abrir https://wokwi.com/.
2. Criar/abrir projeto ESP32 e copiar arquivos (mínimo `sketch.ino`).
3. Conferir SSID padrão: `Wokwi-GUEST` (já no código) e senha vazia.
4. Executar (Play). Manter três janelas paralelas rodando.
5. Cada beacon responde apenas à sua placa:
   - Beacon 1: `ABC1234`
   - Beacon 2: `CDE5678`
   - Beacon 3: `FGH3333`
6. Ao receber comando correto: LED (GPIO 2) acende e buzzer (GPIO 4) toca pulsando.
7. Para desligar: pressionar botão (GPIO 15) simulado (PULLUP → nível LOW aciona desligamento).

### 3. Acionando 🔔
1. No app mobile, toque em "Localizar moto" para a placa desejada.
2. Beacon correspondente ativará alerta.

---

## Fluxo de Dados (Resumo) 🔄
App mobile (Localizar moto) → MQTT (HiveMQ) → Beacons filtram e acionam hardware.

---

## Problemas Comuns 🐛
| Problema | Causa provável | Solução |
|----------|----------------|---------|
| Beacon não reage | Placa enviada diferente da esperada | Verificar placa no payload do botão Node-RED |
| Sem som | Buzzer pin incorreto na simulação | Garantir pino 4 conectado a buzzer ativo no Wokwi |
| MQTT não conecta | Instabilidade broker público ⚠️ | Aguardar, ou trocar broker público / local |
| Muitos registros duplicados | Reenvio repetido de comando | Confirmar não há automação extra publicando |

---

 

---

## Ordem Rápida (Checklist) ✅
1. `node-red` → importar fluxo → abrir `/ui`
2. Abrir 3 projetos Wokwi (beacons) → rodar
3. Clicar em um botão no dashboard
4. Ver logs dos beacons e do broker

---

## Licença 📄
Projeto acadêmico (FIAP) – uso educacional.

---