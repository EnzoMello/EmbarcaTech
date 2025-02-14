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

ssd1306_t display;

// Definições de Rede
#define WIFI_SSID "moto g(9) power_6168"
#define WIFI_PASSWORD "ENZOMELO10"
#define SERVER_PORT 8050

// Variável global para armazenar a temperatura do servidor
float server_temperature = 0.0f;

// Limite de discrepância para alerta
#define TEMPERATURE_ALERT_LIMIT 5.0f  // Limite de 5°C para alerta

// Função para ler a temperatura interna
float read_internal_temperature() {
    uint16_t adc_value = adc_read();
    float voltage = adc_value * (3.3f / (1 << 12)); // 12-bit ADC resolution
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

// Exibir temperaturas no OLED com mensagens ajustadas
void display_temperatures(float sensor_temp, float server_temp, const char* alert_msg) {
    char buffer[32];
    int line = 0;

    ssd1306_clear(&display);

    snprintf(buffer, sizeof(buffer), "Sensor: %.2f C", sensor_temp);
    ssd1306_draw_string(&display, 0, line * 10, 1, buffer);
    line++;

    snprintf(buffer, sizeof(buffer), "Servidor: %.2f C", server_temp);
    ssd1306_draw_string(&display, 0, line * 10, 1, buffer);
    line++;

    // Quebra a mensagem de alerta em várias linhas, se necessário
    int alert_length = strlen(alert_msg);
    int start = 0;
    while (start < alert_length) {
        char temp_msg[32];
        int len = (alert_length - start > 16) ? 16 : alert_length - start;  // Limita a 16 caracteres por linha
        strncpy(temp_msg, alert_msg + start, len);
        temp_msg[len] = '\0';  // Garantir a terminação nula

        ssd1306_draw_string(&display, 0, line * 10, 1, temp_msg);
        line++;

        start += len;
    }

    ssd1306_show(&display);
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
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

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

    // Alterando o valor da temperatura recebida para teste
    server_temperature = 24.84f + 30.0f;  // Temperatura recebida do servidor com incremento de 30°C

    while (true) {
        // Ler a temperatura do sensor interno
        sensor_temperature = read_internal_temperature();

        // Verificar a discrepância e gerar o alerta
        float discrepancy = sensor_temperature - server_temperature;
        if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
            snprintf(alert_message, sizeof(alert_message), "ALERTA: Calor acima do limite seguro de 5 C");
        } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
            snprintf(alert_message, sizeof(alert_message), "ALERTA: Frio abaixo do limite seguro de 5 C");
        } else {
            snprintf(alert_message, sizeof(alert_message), "Temperatura dentro do limite seguro de 5 C");
        }

        // Exibir as temperaturas no OLED
        display_temperatures(sensor_temperature, server_temperature, alert_message);

        // Controlar o LED de alerta (com piscar)
        control_led_alert(sensor_temperature, server_temperature, &led_state, &last_change_time);

        sleep_ms(2000);
    }

    return 0;
}
