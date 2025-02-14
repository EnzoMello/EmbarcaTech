require('dotenv').config();
const express = require('express');
const axios = require('axios');
const net = require('net'); // Usaremos o módulo net para TCP

const app = express();
const PORT = 3000;

const API_KEY = process.env.API_KEY;
const PLACA_IP = '192.168.122.47'; // Substitua pelo IP da sua placa
const PLACA_PORT = 8050; // A mesma porta TCP configurada na sua placa

// Criação de um cliente TCP para enviar dados para a placa
const sendDataToPlaca = (temperatura) => {
    const client = new net.Socket();

    client.connect(PLACA_PORT, PLACA_IP, () => {
        console.log('Conectado à placa');
        // Envia os dados da temperatura
        client.write(`${temperatura}\n`);
    });

    client.on('data', (data) => {
        console.log('Dados recebidos da placa:', data.toString());
        client.destroy(); // Encerra a conexão após receber resposta
    });

    client.on('error', (err) => {
        console.error('Erro na conexão TCP:', err.message);
    });

    client.on('close', () => {
        console.log('Conexão TCP fechada');
    });
};

// Rota para buscar dados da API do clima
app.get('/clima/:cidade', async (req, res) => {
    try {
        const cidade = req.params.cidade;
        const url = `https://api.openweathermap.org/data/2.5/weather?q=${cidade}&appid=${API_KEY}&units=metric&lang=pt`;

        const resposta = await axios.get(url);
        const temperatura = resposta.data.main.temp;

        console.log(`Temperatura em ${cidade}: ${temperatura}°C`);

        // Envia os dados da temperatura para a placa via TCP
        sendDataToPlaca(temperatura);

        res.json({ cidade, temperatura });
    } catch (error) {
        console.error("Erro ao buscar dados da API:", error.message);
        res.status(500).json({ erro: 'Erro ao buscar dados da API' });
    }
});

app.listen(PORT, () => {
    console.log(`Servidor rodando em http://localhost:${PORT}`);
});
