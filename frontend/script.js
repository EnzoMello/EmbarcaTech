async function buscarTemperatura() {
    const cidade = document.getElementById("cidade").value;
    const resultado = document.getElementById("resultado");

    if (!cidade) {
        resultado.innerHTML = "<p style='color: red;'>Por favor, digite uma cidade.</p>";
        return;
    }

    try {
        const resposta = await fetch("http://localhost:3000/temperatura", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ cidade })
        });

        const dados = await resposta.json();

        if (resposta.ok) {
            resultado.innerHTML = `<p>Temperatura em ${dados.cidade}: <strong>${dados.temperatura}°C</strong></p>`;
        } else {
            resultado.innerHTML = `<p style='color: red;'>Erro ao buscar temperatura.</p>`;
        }
    } catch (erro) {
        resultado.innerHTML = `<p style='color: red;'>Erro de conexão.</p>`;
    }
}
