const express = require('express');
const axios = require('axios');
const net = require('net');
const cors = require('cors'); // Importa o CORS
require('dotenv').config();

const app = express();
const PORT = 3000;
const API_KEY = '5d30ae41d98933aef6c4a1dd07a83f69';
const BITDOG_IP = '192.168.122.47'; // Ajuste para o IP da sua placa
const BITDOG_PORT = 8050; // Porta usada para comunicação
const TELEGRAM_BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN;
const TELEGRAM_CHAT_ID = process.env.TELEGRAM_CHAT_ID;
const TEMPERATURE_ALERT_LIMIT = 5; // Defina o limite de variação aceitável

app.use(cors()); // Habilita CORS para permitir requisições do frontend
app.use(express.json());
app.use(express.static('frontend'));

// Debug para verificar se as variáveis estão carregadas corretamente
console.log("TELEGRAM_BOT_TOKEN:", TELEGRAM_BOT_TOKEN);
console.log("TELEGRAM_CHAT_ID:", TELEGRAM_CHAT_ID);

app.post('/temperatura', async (req, res) => {
    const { cidade } = req.body;
    
    if (!cidade) {
        return res.status(400).json({ erro: "Cidade não fornecida" });
    }

    try {
        const url = `https://api.openweathermap.org/data/2.5/weather?q=${cidade}&units=metric&appid=${API_KEY}&lang=pt_br`;
        const resposta = await axios.get(url);
        const temperatura = resposta.data.main.temp;

        console.log(`Temperatura em ${cidade}: ${temperatura}°C`);
        enviarParaPlaca(temperatura);

        // Obtém a temperatura local da placa
        const temperaturaLocal = 20.0;

        if (temperaturaLocal !== null) {
            console.log(`Temperatura local: ${temperaturaLocal}°C`);

            // Compara as temperaturas e decide se envia alerta
            const discrepancy = temperaturaLocal - temperatura;

            let alerta;
            if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
                alerta = `🔥 ALERTA: Calor ambiente em status prejudicial à temperatura em ${cidade}.`;
            } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
                alerta = `❄ ALERTA: ALERTA: Frio ambiente em status prejudicial à temperatura em ${cidade} detectado.`;
            } else {
                alerta = "✅ Clima estável. Nenhum risco detectado.";
            }

            enviarAlertaTelegram(alerta);
        }

        res.json({ cidade, temperatura });
    } catch (erro) {
        res.status(500).json({ erro: "Erro ao buscar temperatura" });
    }
});

function enviarParaPlaca(temperatura) {
    const cliente = new net.Socket();
    
    cliente.connect(BITDOG_PORT, BITDOG_IP, () => {
        console.log(`Enviando temperatura: ${temperatura}°C`);
        cliente.write(temperatura.toString());
        cliente.end();
    });

    cliente.on('error', (err) => {
        console.error('Erro na conexão TCP:', err);
    });
}

app.listen(PORT, () => {
    console.log(`Servidor rodando na porta ${PORT}`);
});

async function enviarAlertaTelegram(mensagem) {
    const url = `https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`

    const resposta = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
            chat_id: TELEGRAM_CHAT_ID,
            text: mensagem
        })
    });

    const dados = await resposta.json();
    if (!dados.ok) {
        console.error("Erro ao enviar mensagem para o Telegram:", dados);
    } else {
        console.log("Mensagem enviada ao Telegram:", mensagem);
    }
}
