#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "hardware/uart.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dhcp.h"
#include "lwip/pbuf.h"
#include "lwip/dns.h"
#
#include "lwip/timeouts.h"
#include "hardware/pwm.h"

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

// BUZZER
#define BUZZER_PIN 10

ssd1306_t display;

// Definições de Rede
#define WIFI_SSID "moto g(9) power_6168"
#define WIFI_PASSWORD "ENZOMELO1000"
#define SERVER_PORT 3000
#define SERVER_IP "192.168.26.35"
#define SERVER_PATH "/enviar-dados"

#define MAX_REQUEST_LEN 512
#define MAX_RESPONSE_LEN 512

// Variável global para armazenar a temperatura do servidor
float server_temperature = 0.0f;
// Variáveis globais pra armazenar estados dos LEDS e usar com os botões
volatile bool led_red_blinking = false;
volatile bool led_blue_blinking = false;

// Limite de discrepância para alerta
#define TEMPERATURE_ALERT_LIMIT 5.0f  // Limite de 10°C para alerta

void display_message(const char *message) {
    char buffer[32];
    int line = 0;

    ssd1306_clear(&display);

    // Quebra a mensagem de alerta em várias linhas, se necessário
    int alert_length = strlen(message);
    int start = 0;
    while (start < alert_length) {
        char temp_msg[32];
        int len = (alert_length - start > 16) ? 17 : alert_length - start;  // Limita a 16 caracteres por linha
        strncpy(temp_msg, message + start, len);
        temp_msg[len] = '\0';  // Garantir a terminação nula

        ssd1306_draw_string(&display, 0, line * 10, 1, temp_msg);
        line++;

        start += len;
    }

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

void tmp_init(){
    adc_init(); // Inicializa o ADC
    adc_gpio_init(28); // Habilita o GPIO 28 como entrada analógica
    adc_select_input(2); // O GPIO 28 corresponde ao canal ADC2
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

// Função para configurar o buzzer como alerta
void buzzer_alert(float discrepancy) {
    if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
        gpio_put(BUZZER_PIN, 1);
        sleep_ms(300);
        gpio_put(BUZZER_PIN, 0);
        sleep_ms(300);
    } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
        gpio_put(BUZZER_PIN, 1);
        sleep_ms(200);
        gpio_put(BUZZER_PIN, 0);
        sleep_ms(200);
    
    } else {
    // Temperatura dentro do limite
    gpio_put(BUZZER_PIN, 0);
    }
}

// Função para configurar o buzzer como entrada
void setup_buzzer() {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_put(BUZZER_PIN, 0);  // Começa desligado
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


    // Resetar os estados dos LEDs
    led_red_blinking = false;
    led_blue_blinking = false;
    
    buzzer_alert(discrepancy);

    if (discrepancy > TEMPERATURE_ALERT_LIMIT) {
        // Alerta de calor - Pisca LED vermelho
        if (current_time - *last_change_time > 200) {  // Piscar a cada 200 ms
            *led_state = !*led_state;  // Alterna o estado do LED
            gpio_put(LED_R_PIN, *led_state);
            gpio_put(LED_B_PIN, 0);
            gpio_put(LED_G_PIN, 0);
            *last_change_time = current_time;
        }
        led_red_blinking = true;
        
    } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
        // Alerta de frio - Pisca LED azul
        if (current_time - *last_change_time > 200) {  // Piscar a cada 200 ms
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

typedef struct {
    struct tcp_pcb *pcb;
    bool complete;
    bool connected;
    char request[MAX_REQUEST_LEN];
    char response[MAX_RESPONSE_LEN];
    size_t response_len;
    ip_addr_t remote_addr;
} HTTP_CLIENT_T;

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_client_close(HTTP_CLIENT_T *state);

static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    HTTP_CLIENT_T *state = (HTTP_CLIENT_T*)arg;

    if (!p) {
        state->complete = true;
        return ERR_OK;
    }

    if (p->tot_len > 0) {
        size_t to_copy = (p->tot_len < MAX_RESPONSE_LEN - state->response_len - 1) ? p->tot_len : MAX_RESPONSE_LEN - state->response_len - 1;
        pbuf_copy_partial(p, state->response + state->response_len, to_copy, 0);
        state->response_len += to_copy;
        state->response[state->response_len] = '\0';
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro ao conectar: %d\n", err);
        return err;
    }

    HTTP_CLIENT_T *state = (HTTP_CLIENT_T*)arg;
    state->connected = true;

    err = tcp_write(tpcb, state->request, strlen(state->request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("Erro ao enviar requisição: %d\n", err);
        return err;
    }

    err = tcp_output(tpcb);
    return err;
}

static void tcp_client_close(HTTP_CLIENT_T *state) {
    if (state->pcb) {
        tcp_close(state->pcb);
        state->pcb = NULL;
    }
}

bool send_http_json(float temperature) {
    temperature = get_temp();

    HTTP_CLIENT_T *state = (HTTP_CLIENT_T*)calloc(1, sizeof(HTTP_CLIENT_T));
    if (!state) {
        printf("Erro ao alocar memória\n");
        return false;
    }

    char json_data[256];
    snprintf(json_data, sizeof(json_data),
             "{\"temperature\": %.2f,}",
            temperature);

    snprintf(state->request, MAX_REQUEST_LEN,
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             SERVER_PATH, SERVER_IP, (int)strlen(json_data), json_data);

    printf("Enviando JSON para API...\n%s\n", state->request);

    ip_addr_t remote_addr;
    if (!ipaddr_aton(SERVER_IP, &remote_addr)) {
        printf("Erro ao resolver IP\n");
        free(state);
        return false;
    }
    state->remote_addr = remote_addr;

    state->pcb = tcp_new();
    if (!state->pcb) {
        printf("Erro ao criar PCB TCP\n");
        free(state);
        return false;
    }

    tcp_arg(state->pcb, state);
    tcp_recv(state->pcb, tcp_client_recv);
    tcp_sent(state->pcb, tcp_client_sent);

    err_t err = tcp_connect(state->pcb, &state->remote_addr, SERVER_PORT, tcp_client_connected);
    if (err != ERR_OK) {
        printf("Erro ao conectar ao servidor: %d\n", err);
        tcp_client_close(state);
        free(state);
        return false;
    }

    uint32_t start_time = to_ms_since_boot(get_absolute_time());
    while (!state->complete && (to_ms_since_boot(get_absolute_time()) - start_time < 10000)) {
        sleep_ms(100);
    }

    bool success = state->complete;
    if (success) {
        printf("Resposta da API: %s\n", state->response);
    } else {
        printf("Falha ao enviar JSON\n");
    }

    tcp_client_close(state);
    free(state);
    return success;
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

    // Inicializar os botões e o buzzer
    setup_buttons();
    setup_buzzer();


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
            snprintf(alert_message, sizeof(alert_message), "ALERTA-> Calor ambiente prejudicial, aperte botao B");
        } else if (discrepancy < -TEMPERATURE_ALERT_LIMIT) {
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