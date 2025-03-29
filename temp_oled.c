#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h" // Leituras dos pinos ADC da placa e seus valores
#include "ssd1306.h" // Controla o display OLED SSD1306 para exibir informações na tela
#include "hardware/uart.h" 
#include "utils/Buttons/Buttons.h"
#include "utils/Buzzer/Buzzer.h"
#include "utils/Display/Display.h"
#include "utils/LED/LED.h"
#include "utils/Tmp36/tmp.h"
#include "utils/WIFI/wifi.h"



#include "pico/cyw43_arch.h" // Controla o módulo Wi-Fi CYW43439 da Raspberry Pi Pico W
#include "lwip/tcp.h" // Gerencia conexões TCP/IP (usado para enviar temperatura para o backend)
#include "lwip/dhcp.h"
#include "lwip/pbuf.h" // Manipula buffers de pacotes de rede (auxilia na transmissão e recepção de dados)
#include "lwip/dns.h"
#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "hardware/pwm.h" // Biblioteca para controlar PWM em alertas sonoros e visuais


#define SERVER_PORT 3000
#define SERVER_IP "192.168.26.35"



// Variável global para armazenar a temperatura do servidor
float server_temperature = 0.0f;

// Variáveis globais pra armazenar estados dos LEDS e usar com os botões
volatile bool led_red_blinking = false;
volatile bool led_blue_blinking = false;



void my_callback_function(uint pin, uint32_t event) {

    // caso a interrupção tenha vindo do botão A
    if (pin == BUTTON_HOT_PIN) {
        if (led_red_blinking) {
            printf("Botão A bloqueado por que não corresponde ao seu LED.");
            return;
        }

        printf("\nbotão B pressionado\n");
        display_message("Fique aquecido e veja o alerta no Telegram.");
    }
    
    // caso a interrupção tenha vindo do botão B
    if(pin == BUTTON_COLD_PIN) {
        if (led_blue_blinking) {
            printf("Botão B bloqueado por que não corresponde ao seu LED.");
            return;
        }

        printf("\nbotão A pressionado\n");
        display_message("Se hidrate e veja o alerta no Telegram.");
    }
}

// Função para controlar o LED RGB (vermelho para calor, azul para frio, piscando)
void control_led_alert(float sensor_temp, float server_temp, int *led_state, int *last_change_time) {
    float discrepancy = sensor_temp - server_temp;

    // Tempo de controle para piscar os LEDs (em milissegundos)
    int current_time = to_ms_since_boot(get_absolute_time());


    // Resetar os estados dos LEDs
    led_red_blinking = false;
    led_blue_blinking = false;
    
    buzzer_alert(discrepancy);



    if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
        // Alerta de calor - Pisca LED vermelho
        if (current_time - *last_change_time > 100) {  // Piscar a cada 200 ms
            *led_state = !*led_state;  // Alterna o estado do LED
            gpio_put(LED_R_PIN, *led_state);
            gpio_put(LED_B_PIN, 0);
            gpio_put(LED_G_PIN, 0);
            *last_change_time = current_time;
        }
        led_red_blinking = true;
    
        
    } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
        // Alerta de frio - Pisca LED azul
        if (current_time - *last_change_time > 100) {  // Piscar a cada 200 ms
            *led_state = !*led_state;  // Alterna o estado do LED
            gpio_put(LED_R_PIN, 0);
            gpio_put(LED_B_PIN, *led_state);
            gpio_put(LED_G_PIN, 0);
            *last_change_time = current_time;
        }
        led_blue_blinking = true;
    } else {
        // Temperatura dentro do limite - LED verde
        gpio_put(LED_R_PIN, 0);
        gpio_put(LED_B_PIN, 0);
        gpio_put(LED_G_PIN, 1);
        led_red_blinking = true;
        led_blue_blinking = true;
    }
}

// Função de callback para receber dados TCP
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p != NULL) {
        // Copiar os dados para um buffer legível
        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, (char*)p->payload, p->len);
        buffer[p->len] = '\0'; // Garantir terminação nula

        // Recebe a temperatura do servidor e converte para float
        server_temperature = atof(buffer);
        printf("Temperatura do servidor recebida: %.2f C\n", server_temperature);

        float temperatura_local = get_temp();  // Suponha que get_temp() retorna a temperatura da placa
        char response[50];
        snprintf(response, sizeof(response), "%.2f", temperatura_local);

        // Enviar temperatura da placa de volta ao servidor
        err_t write_err = tcp_write(tpcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
        if (write_err != ERR_OK) {
            printf("Erro ao enviar temperatura para o servidor\n");
        } else {
            printf("Temperatura local enviada para o servidor: %.2f C\n", temperatura_local);
        }
        // Garantir envio imediato
        tcp_output(tpcb);

        // Liberar buffer
        pbuf_free(p);
    } else {
        // Se p for NULL, a conexão foi fechada
        tcp_close(tpcb);
    }
    return ERR_OK;
}

// Função de callback para gerenciar nova conexão
err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    // Definir a função de recebimento para esta nova conexão
    tcp_recv(newpcb, tcp_recv_callback);
    return ERR_OK;
}

// Função para iniciar o servidor TCP
void tcp_server(void) {
    struct tcp_pcb *pcb;
    err_t err;

    printf("Iniciando servidor TCP...\n");

    // Criar um novo PCB (control block) para o servidor TCP
    pcb = tcp_new();
    if (pcb == NULL) {
        printf("Erro ao criar o PCB TCP.\n");
        return;
    }

    // Vincular o servidor ao endereço e porta desejada
    ip_addr_t ipaddr;
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);  // Ou use IP_ADDR_ANY para todas as interfaces
    err = tcp_bind(pcb, &ipaddr, SERVER_PORT);
    if (err != ERR_OK) {
        printf("Erro ao vincular ao endereço e porta.\n");
        return;
    }

    // Colocar o servidor para ouvir conexões
    pcb = tcp_listen(pcb);
    if (pcb == NULL) {
        printf("Erro ao colocar o servidor em escuta.\n");
        return;
    }

    // Configurar a função de aceitação das conexões
    tcp_accept(pcb, tcp_accept_callback);
    printf("Servidor TCP iniciado na porta %d.\n", SERVER_PORT);
}


void init_components() {
    // Inicializa botões
    setup_button();
    // Inicializa buzzer
    setup_buzzer();
    // Inicializa display
    setup_display();
    // Inicializa LED
    setup_LED();
    // Inicializa sensor de temperatura TMP36
    tmp_init();

}

int main() {
    // Inicializar entradas e saídas padrão
    stdio_init_all();

    init_components(); 

    init_wifi();

    sleep_ms(300);

    // Iniciar servidor TCP
    tcp_server();

    float sensor_temperature = 0.0f;
    char alert_message[128] = "";

    // Variáveis para controlar o estado e tempo de mudança do LED
    int led_state = 0;
    int last_change_time = 0;

    while (true) {
        // Ler a temperatura do sensor interno
        sensor_temperature = get_temp();

        // Verificar a discrepância e gerar o alerta
        float discrepancy = sensor_temperature - server_temperature;
        if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
            snprintf(alert_message, sizeof(alert_message), "ALERTA-> Calor ambiente prejudicial, aperte botao B");
        } else if (discrepancy < TEMPERATURE_ALERT_LIMIT) {
            snprintf(alert_message, sizeof(alert_message), "ALERTA-> Frio ambiente prejudicial, aperte botao A");
        } else {
            snprintf(alert_message, sizeof(alert_message), "Temperatura ambiente segura");
        }

        // Exibir as temperaturas no OLED
        display_temperatures(sensor_temperature, server_temperature, alert_message);

        // Controlar o LED de alerta (com piscar)
        control_led_alert(sensor_temperature, server_temperature, &led_state, &last_change_time);
        tight_loop_contents();      // Mantém o processador ativo para interrupções
        sleep_ms(2000); 
    }

    return 0;
}