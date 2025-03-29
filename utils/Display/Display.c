#include "Display.h"

void setup_display() {
    i2c_init(i2c1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar display SSD1306\n");
        return 1;
    }
}

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

