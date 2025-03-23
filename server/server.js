const express = require('express');
const axios = require('axios');
const net = require('net');
const cors = require('cors'); // Importa o CORS
require('dotenv').config();

const app = express();
const PORT = 3000;
const API_KEY = '5d30ae41d98933aef6c4a1dd07a83f69';
const BITDOG_IP = '192.168.26.47'; // Ajuste para o IP da sua placa
const BITDOG_PORT = 3000; // Porta usada para comunicação
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
    const { cidade, tempLocal } = req.body;
    
    if (!cidade || tempLocal === undefined) {
        return res.status(400).json({ erro: "Cidade e temperatura local são obrigatórias." });
    }

    try {
        const url = `https://api.openweathermap.org/data/2.5/weather?q=${cidade}&units=metric&appid=${API_KEY}&lang=pt_br`;
        const resposta = await axios.get(url);
        
        const temperatura = resposta.data.main.temp;
        const dataAtual = new Date().toLocaleDateString('pt-BR');
        const horaAtual = new Date().toLocaleTimeString('pt-BR');

        console.log(`Temperatura em ${cidade}: ${temperatura}°C`);
        enviarParaPlaca(temperatura);

        // Obtém a temperatura local da placa
        const temperaturaLocal = tempLocal;


        if (temperaturaLocal !== null) {
            console.log(`Temperatura local: ${temperaturaLocal}°C`);

            // Compara as temperaturas e decide se envia alerta
            const discrepancy = temperaturaLocal - temperatura;

            let alerta;
            if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
                alerta = `${gerarMensagemCalor(cidade, temperatura, temperaturaLocal, dataAtual, horaAtual, discrepancy)}`;
            } else if (discrepancy < TEMPERATURE_ALERT_LIMIT) {
                alerta = `${gerarMensagemFrio(cidade, temperatura, temperaturaLocal, dataAtual, horaAtual, discrepancy)}`;
            } else {
                alerta = "✅ Clima estável. Nenhum risco detectado.";
            }

            await enviarAlertaTelegram(alerta);
        }

        res.json({ cidade, temperatura });
    } catch (erro) {
        console.log(erro.message);
        res.status(500).json({ erro: "Erro ao buscar temperatura" });
    }
});

function enviarParaPlaca(temperatura) {
    const cliente = new net.Socket();
    
    cliente.connect(BITDOG_PORT, BITDOG_IP, () => {
        console.log(`Enviando temperatura: ${temperatura}°C`);
        cliente.write(temperatura.toString());
    });

    cliente.on('data', (data) => {
        const temperaturaRecebida = parseFloat(data.toString()); // Converte a resposta para número
        if (!isNaN(temperaturaRecebida)) {
            tempLocal = temperaturaRecebida; // Atualiza a variável global
            console.log(`⚡ Temperatura local atualizada: ${tempLocal}°C`);
        } else {
            console.warn('🚨 Dados recebidos da placa não são numéricos:', data.toString());
        }
    });
    

    cliente.on('error', (err) => {
        console.error('Erro na conexão TCP:', err);
    });

    cliente.on('close', () => {
        console.log('Conexão encerrada.');
    });
}

app.listen(PORT, () => {
    console.log(`Servidor rodando na porta ${PORT}`);
});

async function enviarAlertaTelegram(mensagem) {
    const url = `https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage`;

    try {
        const resposta = await axios.post(url, {
            chat_id: TELEGRAM_CHAT_ID,
            text: mensagem
        });

        if (!resposta.data.ok) {
            console.error("Erro ao enviar mensagem para o Telegram:", resposta.data);
        } else {
            console.log("Mensagem enviada ao Telegram:", mensagem);
        }
    } catch (error) {
        console.error("Erro ao conectar ao Telegram:", error.message);
    }
}


function getTemperatureFromBoard() {
    return new Promise((resolve, reject) => {
        const client = new net.Socket();

        client.connect(3000, '192.168.26.47', () => {  // Substitua pelo IP real da placa
            console.log("Conectado à placa, solicitando temperatura...");
            client.write("GET_TEMP");  // Mensagem simbólica, a placa ignora isso
        });

        client.on('data', (data) => {
            console.log("Temperatura recebida da placa:", data.toString());
            client.destroy();  // Fecha conexão
            resolve(JSON.parse(data.toString()).temperature);
        });

        client.on('error', (err) => {
            console.error("Erro na conexão com a placa:", err);
            reject(err);
        });

        client.on('close', () => {
            console.log("Conexão encerrada");
        });
    });
}

function gerarMensagemCalor(cidade, temperatura, temperaturaLocal, data, hora, discrepancy) {
    return ` 🚨 ALERTA DE CALOR AMBIENTE PREJUDICIAL 🚨

📌 A temperatura do seu local atual se encontra em um estado acima da temperatura em ${cidade} e do padrão seguro de discrepância, ou seja, o calor excessivo pode prejudicar sua saúde.

📍 Localização ${cidade}
📅 Data ${data}
🕒 Horário ${hora}

🌡️ Temperatura em ${cidade} ${temperatura}°C  
🌡️ Temperatura no seu local atual ${temperaturaLocal}°C  
📊 Variação detectada ${discrepancy.toFixed(1)}°C  

⚠️ Riscos à saúde  
✔ Exaustão térmica (tontura, suor excessivo, desmaios).  
✔ Golpe de calor, que pode levar à falência de órgãos.  
✔ Desidratação severa e aumento da frequência cardíaca**.  

✅ Dicas para se proteger  
- 🥤 Hidrate-se (beba pelo menos 2L de água por dia).  
- 👕 Use roupas leves e de cores claras.  
- ⛱️ Evite exposição ao sol entre 10h e 16h.  
- ❄️ Permaneça em locais frescos e ventilados.  

📌 Mais informações:  
🔗 [Como se proteger do calor extremo](https://www.bbc.com/portuguese/articles/cglpg27l811o)
    `
}

function gerarMensagemFrio(cidade, temperatura, temperaturaLocal, data, hora, discrepancy) {
    return ` 🚨 ALERTA DE FRIO AMBIENTE PREJUDICIAL 🚨

📌 A temperatura do seu local atual se encontra em um estado abaixo da temperatura em ${cidade} e do padrão seguro de discrepância, ou seja, o frio excessivo pode prejudicar sua saúde.

📍 Localização ${cidade}
📅 Data ${data}
🕒 Horário ${hora}

🌡️ Temperatura ambiente ${temperatura}°C  
🌡️ Temperatura local ${temperaturaLocal}°C  
📊 Variação detectada ${discrepancy.toFixed(1)}°C  

⚠️ Riscos à saúde  
✔ ✔ Hipotermia (queda da temperatura corporal abaixo do normal).  
✔ Risco de doenças respiratórias e cardiovasculares.  
✔ Dormência e contração dos músculos.  

✅ Dicas para se proteger:  
- 🧥 Use roupas quentes e bem isoladas.  
- ☕ Beba líquidos quentes e evite bebidas alcoólicas.  
- 🏡 Fique em locais aquecidos sempre que possível.  

📌 Mais informações:  
🔗 [Como se proteger do calor extremo](https://www.nationalgeographicbrasil.com/ciencia/2024/07/como-evitar-que-o-frio-extremo-afete-a-saude-humana)
    `
}


