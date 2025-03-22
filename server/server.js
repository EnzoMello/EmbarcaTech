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
let tempLocal = 20.0;


app.use(cors()); // Habilita CORS para permitir requisições do frontend
app.use(express.json());
app.use(express.static('frontend'));

// Debug para verificar se as variáveis estão carregadas corretamente
console.log("TELEGRAM_BOT_TOKEN:", TELEGRAM_BOT_TOKEN);
console.log("TELEGRAM_CHAT_ID:", TELEGRAM_CHAT_ID);

app.post('/temperatura/local', (req, res) => {
    const { temperaturaLocal } = req.body;

    if (temperaturaLocal === undefined || temperaturaLocal === null) {
        return res.status(400).json({ erro: "Temperatura local não fornecida" });
    }

    tempLocal = temperaturaLocal; // Armazena a temperatura recebida

    console.log(`Temperatura local atualizada: ${temperaturaLocal}°C`);

    res.json({ mensagem: "Temperatura local recebida com sucesso", temperaturaLocal });
});

app.post('/api/temperatura', (req, res) => {
    const temperatura = req.body.temperature;  // O nome da chave do JSON é "temperature"
  
    if (temperatura !== undefined) {
      console.log(`Temperatura recebida: ${temperatura}°C`);
      
      tempLocal = temperatura;
  
      res.status(200).send({ message: "Temperatura recebida com sucesso!" });
    } else {
      res.status(400).send({ message: "Temperatura não encontrada no corpo da requisição!" });
    }
  });

app.post('/temperatura', async (req, res) => {
    const { cidade } = req.body;
    
    if (!cidade) {
        return res.status(400).json({ erro: "Cidade não fornecida" });
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
            } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
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


