const express = require('express');
const axios = require('axios');
const net = require('net');
const cors = require('cors'); // Importa o CORS

const app = express();
const PORT = 3000;
const API_KEY = '5d30ae41d98933aef6c4a1dd07a83f69';
const BITDOG_IP = '192.168.122.47'; // Ajuste para o IP da sua placa
const BITDOG_PORT = 8050; // Porta usada para comunicação

app.use(cors()); // Habilita CORS para permitir requisições do frontend
app.use(express.json());
app.use(express.static('frontend'));

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
