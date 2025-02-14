#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "hardware/uart.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/timeouts.h"

// Definições do display SSD1306
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define I2C_SDA 14
#define I2C_SCL 15

// LED RGB
#define LED_R_PIN 13
#define LED_B_PIN 12
#define LED_G_PIN 11

// BOTÕES
#define BUTTON_HOT_PIN 5
#define BUTTON_COLD_PIN 6

ssd1306_t display;

// Definições de Rede
#define WIFI_SSID "moto g(9) power_6168"
#define WIFI_PASSWORD "ENZOMELO10"
#define SERVER_PORT 8050

// Variável global para armazenar a temperatura do servidor
float server_temperature = 0.0f;

// Limite de discrepância para alerta
#define TEMPERATURE_ALERT_LIMIT 10.0f  // Limite de 5°C para alerta

void display_message(const char *message) {
    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 10, 25, 1, message);
    ssd1306_show(&display);
}


// Exibir temperaturas no OLED com mensagens ajustadas
void display_temperatures(float sensor_temp, float server_temp, const char* alert_msg) {
    char buffer[32];
    int line = 0;

    ssd1306_clear(&display);

    snprintf(buffer, sizeof(buffer), "Local atual: %.2f C", sensor_temp);
    ssd1306_draw_string(&display, 0, line * 10, 1, buffer);
    line++;

    snprintf(buffer, sizeof(buffer), "Cidade atual: %.2f C", server_temp);
    ssd1306_draw_string(&display, 0, line * 10, 1, buffer);
    line++;

    // Quebra a mensagem de alerta em várias linhas, se necessário
    int alert_length = strlen(alert_msg);
    int start = 0;
    while (start < alert_length) {
        char temp_msg[32];
        int len = (alert_length - start > 16) ? 17 : alert_length - start;  // Limita a 16 caracteres por linha
        strncpy(temp_msg, alert_msg + start, len);
        temp_msg[len] = '\0';  // Garantir a terminação nula

        ssd1306_draw_string(&display, 0, line * 10, 1, temp_msg);
        line++;

        start += len;
    }

    ssd1306_show(&display);
}

void my_callback_function(uint pin, uint32_t event) {

    // caso a interrupção tenha vindo do botão A
    if (pin == BUTTON_HOT_PIN) {
        printf("\nbotão B pressionado\n");
        display_message("Mantenha-se hidratado!");
    }
    
    // caso a interrupção tenha vindo do botão B
    if(pin == BUTTON_COLD_PIN) {
        printf("\nbotão A pressionado\n");
        display_message("Mantenha-se aquecido!");
    }
}

void tmp_init(){
    adc_init(); // Inicializa o ADC
    adc_gpio_init(28); // Habilita o GPIO 18 como entrada analógica
    adc_select_input(2); // O GPIO 18 corresponde ao canal ADC2
};

float get_temp() {
    int num_readings = 10;  // Número de leituras para a média
    float sum = 0.0f;

    // Ler múltiplos valores e somá-los
    for (int i = 0; i < num_readings; i++) {
        uint16_t raw_value = adc_read();
        float voltage = (raw_value * 3.3f) / (1 << 12);
        sum += voltage;
        sleep_ms(10);  // Espera um pouco entre as leituras para evitar leituras muito rápidas
    }

    // Calcular a média das leituras
    float avg_voltage = sum / num_readings;

    // Aplicar um fator de escala para reduzir a sensibilidade
    float temperature = (avg_voltage - 0.5f) / (0.02f * 2); // Diminuir a sensibilidade multiplicando por 2

    return temperature;
}



// Função para configurar os botões como entradas
void setup_buttons() {
    gpio_init(BUTTON_HOT_PIN);
    gpio_set_dir(BUTTON_HOT_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_HOT_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_HOT_PIN, GPIO_IRQ_EDGE_FALL, true, &my_callback_function);

    gpio_init(BUTTON_COLD_PIN);
    gpio_set_dir(BUTTON_COLD_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_COLD_PIN);
    gpio_set_irq_enabled(BUTTON_COLD_PIN, GPIO_IRQ_EDGE_FALL, true);
}

// Função para controlar o LED RGB (vermelho para calor, azul para frio, piscando)
void control_led_alert(float sensor_temp, float server_temp, int *led_state, int *last_change_time) {
    float discrepancy = sensor_temp - server_temp;

    // Tempo de controle para piscar os LEDs (em milissegundos)
    int current_time = to_ms_since_boot(get_absolute_time());

    if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
        // Alerta de calor - Pisca LED vermelho
        if (current_time - *last_change_time > 200) {  // Piscar a cada 200 ms
            *led_state = !*led_state;  // Alterna o estado do LED
            gpio_put(LED_R_PIN, *led_state);
            gpio_put(LED_B_PIN, 0);
            gpio_put(LED_G_PIN, 0);
            *last_change_time = current_time;
        }
    } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
        // Alerta de frio - Pisca LED azul
        if (current_time - *last_change_time > 200) {  // Piscar a cada 200 ms
            *led_state = !*led_state;  // Alterna o estado do LED
            gpio_put(LED_R_PIN, 0);
            gpio_put(LED_B_PIN, *led_state);
            gpio_put(LED_G_PIN, 0);
            *last_change_time = current_time;
        }
    } else {
        // Temperatura dentro do limite - LED verde
        gpio_put(LED_R_PIN, 0);
        gpio_put(LED_B_PIN, 0);
        gpio_put(LED_G_PIN, 1);
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

int main() {
    // Inicializar entradas e saídas padrão
    stdio_init_all();

    tmp_init();
    

    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializar display OLED
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar display SSD1306\n");
        return 1;
    }

    // Inicializar LEDs RGB
    gpio_init(LED_R_PIN);
    gpio_init(LED_B_PIN);
    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);
    gpio_set_dir(LED_B_PIN, GPIO_OUT);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);

    // Inicializar os botões
    setup_buttons();

    // Inicializar Wi-Fi
    printf("Iniciando WiFi...\n");
    cyw43_arch_init();
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Falha ao conectar ao WiFi.\n");
        return 1;
    } else {
        printf("Conectado ao WiFi.\n");
    }

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
            snprintf(alert_message, sizeof(alert_message), "ALERTA-> Calor local prejudicial, aperte botao B");
        } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
            snprintf(alert_message, sizeof(alert_message), "ALERTA-> Frio local prejudicial, aperte botao A");
        } else {
            snprintf(alert_message, sizeof(alert_message), "Temperatura ambiente segura");
        }

        // Exibir as temperaturas no OLED
        display_temperatures(sensor_temperature, server_temperature, alert_message);

        // Controlar o LED de alerta (com piscar)
        control_led_alert(sensor_temperature, server_temperature, &led_state, &last_change_time);
        tight_loop_contents();      // Mantém o processador ativo para interrupções
        sleep_ms(1000); 
    }

    return 0;
}