require('dotenv').config();
const express = require('express');
const axios = require('axios');
const { SerialPort, ReadlineParser } = require('serialport'); // Importação correta

const app = express();
const PORT = 3000;

const API_KEY = process.env.API_KEY;
const SERIAL_PORT = 'COM6'; 

const port = new SerialPort({ path: SERIAL_PORT, baudRate: 115200 }); // Definição correta da porta
const parser = port.pipe(new ReadlineParser({ delimiter: '\n' })); // Criação correta do parser

// Evento para verificar se a porta serial abriu corretamente
port.on('open', () => {
    console.log(`Porta Serial ${SERIAL_PORT} aberta com sucesso!`);
});

port.on('error', (err) => {
    console.error(`Erro na porta serial: ${err.message}`);
});

// Rota para buscar dados da API do clima
app.get('/clima/:cidade', async (req, res) => {
    try {
        const cidade = req.params.cidade;
        const url = `https://api.openweathermap.org/data/2.5/weather?q=${cidade}&appid=${API_KEY}&units=metric&lang=pt`;

        const resposta = await axios.get(url);
        const temperatura = resposta.data.main.temp;

        console.log(`Temperatura em ${cidade}: ${temperatura}°C`);

        // Verifica se a porta está aberta antes de escrever
        if (port.isOpen) {
            port.write(`${temperatura}\n`, (err) => {
                if (err) {
                    console.error("Erro ao enviar temperatura para a placa:", err.message);
                } else {
                    console.log("Temperatura enviada com sucesso para a placa.");
                }
            });
        } else {
            console.error("Porta serial não está aberta.");
        }

        res.json({ cidade, temperatura });
    } catch (error) {
        console.error("Erro ao buscar dados da API:", error.message);
        res.status(500).json({ erro: 'Erro ao buscar dados da API' });
    }
});

app.listen(PORT, () => {
    console.log(`Servidor rodando em http://localhost:${PORT}`);
});
