# ClimaSafe - Monitoramento Inteligente de Temperatura

## 📌 Descrição do Projeto
O **ClimaSafe** é um sistema IoT embarcado projetado para monitorar a temperatura do ambiente em tempo real e detectar discrepâncias climáticas prejudiciais à saúde humana. O sistema compara a temperatura ambiente com a temperatura da cidade onde o usuário se encontra, gerando alertas em caso de risco. O projeto tem um foco especial na proteção de indivíduos vulneráveis, como idosos, crianças e doentes crônicos.

---

## 🛠 Tecnologias Utilizadas
- 🚀 **Microcontrolador RP2040 (BitDogLab)**
- 🌡 **Sensor de temperatura TMP36**
- 📟 **Display OLED SSD1306**
- 💡 **LED RGB WS2812B**
- 🔊 **Buzzer sonoro passivo**
- 🎛 **Botões Push-Buttons**
- 🌍 **API OpenWeather** para obtenção da temperatura da cidade
- 🔄 **Comunicação TCP/IP** para troca de dados entre os dispositivos
- 🤖 **Chatbot Telegram** para envio de alertas imediatos
- 💾 **Servidor Backend** (desenvolvido em **Python/Node.js**)
- 🎨 **Frontend HTML, CSS e JavaScript** para interatividade com o usuário

---

## 🔥 Funcionalidades
✔ **Monitoramento Contínuo**: Captura e exibe a temperatura ambiente no display OLED.
✔ **Comparativo Climático**: Obtém a temperatura da cidade informada pelo usuário via API OpenWeather e compara com a temperatura local.
✔ **Geração de Alertas**:
   - Notifica o usuário via **Telegram Bot** sobre variações críticas de temperatura.
   - Ativa **LEDs coloridos** e **alertas sonoros** para avisos visuais e auditivos.
   - Exibe mensagens no **display OLED** com instruções de precaução.
✔ **Interatividade**:
   - O usuário pode informar a cidade pelo **frontend web** para obter a comparação climática.
   - Botões push da BitDogLab permitem interação e mudança de mensagens no display OLED.

---

## 🚀 Instalação e Execução
### ✅ Requisitos
- Placa **BitDogLab**
- Sensor **TMP36**
- **Node.js** (com o servidor backend em Node.js)
- Biblioteca **Requests** para API OpenWeather
- Biblioteca **MQTT** para comunicação (opcional)

### 📥 Passos
1. Clone o repositório:
   ```bash
   git clone https://github.com/EnzoMello/Embarca_Tech.git
   ```
2. Instale as dependências do backend:
   - Usando **Node.js**:
     ```bash
     npm install
     ```
3. Configure suas credenciais da API OpenWeather e do Bot Telegram no arquivo de ambiente.
4. Conecte a placa **BitDogLab** ao computador e carregue o firmware C.
5. Execute o servidor backend:
   ```bash
   node server.js
   ```
6. Acesse a interface web para configurar a cidade e visualizar os dados.

---

## 📌 Justificativa do Projeto
As mudanças climáticas estão intensificando eventos extremos, colocando em risco indivíduos vulneráveis. O **ClimaSafe** é uma solução de baixo custo e fácil implementação, que alerta sobre riscos como desidratação, exaustão térmica e hipotermia, contribuindo para a segurança climática.
---

## 👤 Autor
**Enzo Melo Araújo**

---

## 🔗 Links
📌 Apresentação do projeto: [YouTube](https://youtu.be/xepayADzS1g)
📌 Documento com mais detalhes: [Docs](https://docs.google.com/document/d/11wip6-9nIQfZ_sC0BsJuR4Pn28gB_mGIzb26LRrdy5o/edit?usp=sharing)

---
