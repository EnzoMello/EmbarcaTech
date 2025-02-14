const socket = new WebSocket("ws://localhost:3000");

socket.onopen = () => {
    console.log("Conectado ao servidor WebSocket.");
    document.getElementById("status").textContent = "Aguardando dados da placa...";
};

socket.onmessage = (event) => {
    try {
        const dados = JSON.parse(event.data);
        console.log("Dados recebidos:", dados);

        // Verifica se os dados são válidos
        if (dados.local !== undefined && dados.servidor !== undefined) {
            // Atualiza os elementos HTML com as temperaturas
            document.getElementById("tempLocal").textContent = `${dados.local}°C`;
            document.getElementById("tempServidor").textContent = `${dados.servidor}°C`;

            // Remove a mensagem de status após receber os dados
            document.getElementById("status").textContent = "Dados recebidos com sucesso!";
        } else {
            document.getElementById("status").textContent = "Erro ao processar os dados.";
        }
    } catch (error) {
        console.error("Erro ao processar os dados recebidos:", error);
        document.getElementById("status").textContent = "Falha ao processar dados.";
    }
};

socket.onerror = (error) => {
    console.error("Erro no WebSocket:", error);
    document.getElementById("status").textContent = "Erro na conexão com o servidor.";
};

socket.onclose = () => {
    console.warn("Conexão com WebSocket fechada.");
    document.getElementById("status").textContent = "Conexão encerrada.";
};
