#include "Server.h"


err_t tcp_client_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    
    if (err != ERR_OK || p == NULL) {
        if (p != NULL) pbuf_free(p);
        tcp_close(pcb);
        printf("Conexão fechada\n");
        return ERR_OK;
    }
        
    fwrite(p->payload, 1, p->len, stdout);
    printf("\n");

    pbuf_free(p);

    return ERR_OK; 
}


void send_temp_to_server(const char *path, char *request_body, const char *type_method) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // const char *proxy_host = PROXY_HOST;  // Proxy gerado pelo Railway
    int proxy_port = SERVER_PORT;          // Porta do proxy
    const char *host = SERVER_IP;

    ip_addr_t server_ip;
    IP4_ADDR(&server_ip, server_ip_address[0], server_ip_address[1], server_ip_address[2], server_ip_address[3]); // ip from server

    // Conectando ao servidor
    if (tcp_connect(pcb, &server_ip, SERVER_PORT, NULL) != ERR_OK) {
        printf("Erro ao conectar ao servidor\n");
        tcp_abort(pcb);
        return;
    }

    printf("Conectado ao Servidor!\n");

    // Montando requisição
    char request[521];
    snprintf(request, sizeof(request),  
    "%s %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: %d\r\n"
    "\r\n"  // <- Linha separadora obrigatória antes do corpo da requisição
    "%s"
    "\r\n", // <- Certifique-se de finalizar corretamente
    type_method, path, host, strlen(request_body), request_body);
    
    // Enviando a requisição
    if (tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY) != ERR_OK) {
        printf("Erro ao enviar dados\n");
        tcp_abort(pcb);
        return;
    }

    // Tratando possíveis erros ao enviar requisição
    if (tcp_output(pcb) != ERR_OK) {
        printf("Erro ao enviar dados (tcp_output)\n");
        tcp_abort(pcb);
        return;
    }

    // Mapeando callback para resposta do servidor
    tcp_recv(pcb, tcp_client_recv);
}


void create_request(float temperature) {

    // Configurando método da requisição e o endoint a ser acessado
    const char *typeMethod = "POST";
    const char *path = "/receiveTemp";
    char json_request[256];

    // Preparando o corpo da requisição
    snprintf(json_request, sizeof(json_request),
    "{\n"
    "\"temperatura\" : \"%f\""
    "}", temperature);

    // Enviando requisição para o servidor
    send_temp_to_server(path, json_request, typeMethod);
}