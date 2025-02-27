#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"

// Configuração do barramento I2C para o display OLED
#define PORTA_I2C i2c1
#define SDA 14
#define SCL 15
#define ENDERECO_DISPLAY 0x3C

// Definição dos LEDs
#define LED_VERMELHO 13
#define LED_AMARELO 11
#define LED_AZUL 12

// Configuração do joystick
#define EIXO_HORIZONTAL 26  // Pino ADC para o eixo X
#define EIXO_VERTICAL 27    // Pino ADC para o eixo Y
#define BOTAO_JOYSTICK 22   // Botão do joystick
#define BOTAO_EXTRA 5       // Botão extra para simular ambiente quente
#define BOTAO_DESLIGAR 6    // Botão para desligar o alarme

// Configuração do buzzer
#define BUZZER 21

// Tempo de inatividade (em segundos)
#define TEMPO_ALERTA1 5
#define TEMPO_ALERTA2 10
#define TEMPO_ALERTA3 15

// Zona morta para o joystick (evitar falsos positivos)
#define ZONA_MORTA 300

// Tempo necessário para segurar o botão de desligar (em segundos)
#define TEMPO_DESLIGAR 3

// Variáveis de controle
uint32_t tempo_inatividade = 0;
uint16_t tempo_joystick_inativo = 0;
bool alerta_ativado = false;
bool ambiente_quente = false; // Flag para simular ambiente quente
uint32_t tempo_botao_desligar = 0; // Tempo que o botão de desligar foi pressionado
int temperatura = 20; // Temperatura inicial

// Variáveis para o centro dinâmico
uint16_t centro_x = 0;
uint16_t centro_y = 0;
bool centro_definido = false;  // Flag para indicar se o centro foi definido

ssd1306_t tela;

// Função para limpar o display
void ssd1306_clear(ssd1306_t *tela) {
    for (uint16_t i = 1; i < tela->bufsize; ++i) {
        tela->ram_buffer[i] = 0x00;
    }
    ssd1306_send_data(tela);
}

// Função para controlar os LEDs
void controlar_leds(uint8_t vermelho, uint8_t amarelo, uint8_t azul) {
    gpio_put(LED_VERMELHO, vermelho);
    gpio_put(LED_AMARELO, amarelo);
    gpio_put(LED_AZUL, azul);
    printf("LEDs: Vermelho=%d, Amarelo=%d, Azul=%d\n", vermelho, amarelo, azul);  // Depuração
}

// Função para tocar uma nota no buzzer
void buzzer_tone(uint gpio, int duration_ms, int frequency_hz) {
    for (int i = 0; i < duration_ms * frequency_hz / 1000; i++) {
        gpio_put(gpio, 1); // Liga o buzzer
        busy_wait_us(500000 / frequency_hz); // Espera metade do período
        gpio_put(gpio, 0); // Desliga o buzzer
        busy_wait_us(500000 / frequency_hz); // Espera a outra metade
    }
}

// Função para tocar a música "Super Mario Bros"
void tocar_musica() {
    int melodia[] = {659, 659, 0, 659, 0, 523, 659, 0, 784}; // Notas da música
    int duracao[] = {200, 200, 200, 200, 200, 200, 200, 200, 400}; // Duração das notas

    for (int i = 0; i < 9; i++) {
        if (melodia[i] == 0) {
            busy_wait_ms(duracao[i]); // Pausa
        } else {
            buzzer_tone(BUZZER, duracao[i], melodia[i]); // Toca a nota
        }
        busy_wait_ms(50); // Intervalo entre notas
    }
}

// Função para iniciar alertas
void iniciar_alerta(uint8_t nivel) {
    switch (nivel) {
        case 1: 
            // Nível 1 – Aviso Inicial (Fadiga Leve)
            controlar_leds(0, 1, 0);  // LED Amarelo
            buzzer_tone(BUZZER, 500, 261);  // Buzzer com som baixo (Dó)
            break;
        case 2: 
            // Nível 2 – Sonolência Confirmada (Alerta Médio)
            controlar_leds(1, 0, 0);  // LED Vermelho
            buzzer_tone(BUZZER, 500, 523);  // Buzzer aumenta a intensidade (Dó alto)
            break;
        case 3: 
            // Nível 3 – Cochilo Confirmado (Emergência)
            controlar_leds(1, 0, 1);  // LED Vermelho e Azul piscando
            tocar_musica();  // Toca a música "Super Mario Bros"
            break;
        default:
            // Caso não haja alerta, desliga tudo
            controlar_leds(0, 0, 0);  // LEDs desligados
            gpio_put(BUZZER, 0);
            break;
    }
}

// Função para exibir informações no display
void exibir_informacoes(uint16_t adc_x, uint16_t adc_y, uint16_t tempo_joystick_inativo, uint8_t nivel_alerta, bool ambiente_quente, int temperatura) {
    char str_x[10], str_y[10], str_joystick[10], str_alerta[20], str_ambiente[20], str_temperatura[20];

    // Converte os valores para strings
    sprintf(str_x, "X: %d", adc_x);
    sprintf(str_y, "Y: %d", adc_y);
    sprintf(str_joystick, "Joy: %d s", tempo_joystick_inativo);
    sprintf(str_temperatura, "Temp: %d C", temperatura);

    // Define o texto do alerta com base no nível
    switch (nivel_alerta) {
        case 1:
            sprintf(str_alerta, "Fadiga Leve");
            break;
        case 2:
            sprintf(str_alerta, "Sonolencia");
            break;
        case 3:
            sprintf(str_alerta, "PERIGO!");
            break;
        default:
            sprintf(str_alerta, "Normal");
            break;
    }

    // Define o texto do ambiente com base na temperatura
    if (temperatura > 40) {
        sprintf(str_ambiente, "PERIGO! Muito Quente");
    } else if (temperatura > 30) {
        sprintf(str_ambiente, "Ambiente Quente");
    } else if (temperatura < 15) {
        sprintf(str_ambiente, "Ambiente Frio");
    } else {
        sprintf(str_ambiente, "Ambiente Normal");
    }

    // Limpa o display
    ssd1306_clear(&tela);

    // Desenha um retângulo ao redor da área de informações
    ssd1306_rect(&tela, 2, 2, 124, 60, true, false);

    // Exibe as informações no display
    ssd1306_draw_string(&tela, "  Sistema  ", 10, 8);
    ssd1306_draw_string(&tela, str_x, 10, 20);
    ssd1306_draw_string(&tela, str_temperatura, 10, 30); // Exibe a temperatura
    ssd1306_draw_string(&tela, str_alerta, 10, 40); // Exibe o tipo de alerta
    ssd1306_draw_string(&tela, str_ambiente, 10, 50); // Exibe o estado do ambiente

    // Atualiza o display
    ssd1306_send_data(&tela);
}

// Função para verificar o joystick
void verificar_joystick() {
    uint16_t adc_value_x, adc_value_y;
    adc_select_input(1); // Seleciona o ADC para eixo X
    adc_value_x = adc_read();
    adc_select_input(0); // Seleciona o ADC para eixo Y
    adc_value_y = adc_read();

    // Define o centro dinamicamente na primeira leitura
    if (!centro_definido) {
        centro_x = adc_value_x;
        centro_y = adc_value_y;
        centro_definido = true;
        printf("Centro definido: X=%d, Y=%d\n", centro_x, centro_y);
    }

    // Verifica se o joystick está dentro da zona morta
    if (abs(adc_value_x - centro_x) < ZONA_MORTA && abs(adc_value_y - centro_y) < ZONA_MORTA) {
        tempo_joystick_inativo++;
        printf("Joystick inativo: %d segundos\n", tempo_joystick_inativo);
    } else {
        // Reset dos tempos de inatividade e alertas ao mover o joystick
        tempo_joystick_inativo = 0;
        tempo_inatividade = 0;
        alerta_ativado = false;
        printf("Joystick movido: X=%d, Y=%d\n", adc_value_x, adc_value_y);
    }
}

// Função principal
int main() {
    stdio_init_all();
    adc_init();
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);

    gpio_init(BOTAO_EXTRA);
    gpio_set_dir(BOTAO_EXTRA, GPIO_IN);
    gpio_pull_up(BOTAO_EXTRA);

    gpio_init(BOTAO_DESLIGAR);
    gpio_set_dir(BOTAO_DESLIGAR, GPIO_IN);
    gpio_pull_up(BOTAO_DESLIGAR);

    // Configuração dos LEDs
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_init(LED_AMARELO);
    gpio_set_dir(LED_AMARELO, GPIO_OUT);
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);

    // Configuração do buzzer
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);

    // Configuração do display OLED
    i2c_init(PORTA_I2C, 400 * 1000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);
    ssd1306_init(&tela, 128, 64, false, ENDERECO_DISPLAY, PORTA_I2C);
    ssd1306_config(&tela);

    uint32_t tempo_ultimo_atividade = to_ms_since_boot(get_absolute_time());

    while (true) {
        uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());

        // Verifica a cada segundo
        if (tempo_atual - tempo_ultimo_atividade > 1000) {
            tempo_inatividade++;
            tempo_ultimo_atividade = tempo_atual;
            printf("Tempo de inatividade: %d segundos\n", tempo_inatividade);
        }

        // Verificação de atividade do botão e do joystick
        if (!gpio_get(BOTAO_JOYSTICK)) {
            tempo_inatividade = 0;  // Reseta o tempo de inatividade se o botão for pressionado
            printf("Botao pressionado: Resetando tempo de inatividade\n");
        }

        // Simula um ambiente quente e abafado ao pressionar o botão extra
        if (!gpio_get(BOTAO_EXTRA)) {
            ambiente_quente = !ambiente_quente; // Alterna entre ambiente quente e normal
            printf("Ambiente quente: %s\n", ambiente_quente ? "Ativado" : "Desativado");
            sleep_ms(500);  // Debounce
        }

        // Verifica se o botão de desligar foi pressionado e segurado
        if (!gpio_get(BOTAO_DESLIGAR)) {
            tempo_botao_desligar++;
            printf("Botao desligar pressionado: %d segundos\n", tempo_botao_desligar);
            if (tempo_botao_desligar >= TEMPO_DESLIGAR) {
                // Desliga o alarme e retorna ao estado normal
                tempo_inatividade = 0;
                tempo_joystick_inativo = 0;
                alerta_ativado = false;
                ambiente_quente = false;
                printf("Alarme desligado\n");
                tempo_botao_desligar = 0; // Reseta o contador
            }
        } else {
            tempo_botao_desligar = 0; // Reseta o contador se o botão não estiver pressionado
        }

        // Aumenta a temperatura se o botão no pino 5 for pressionado
        if (!gpio_get(BOTAO_EXTRA)) {
            temperatura++;
            printf("Temperatura aumentada: %d C\n", temperatura);
            sleep_ms(500);  // Debounce
        }

        // Diminui a temperatura se o botão no pino 6 for pressionado
        if (!gpio_get(BOTAO_DESLIGAR)) {
            temperatura--;
            printf("Temperatura diminuida: %d C\n", temperatura);
            sleep_ms(500);  // Debounce
        }

        verificar_joystick();

        // Determina o nível de alerta atual
        uint8_t nivel_alerta = 0;
        if (temperatura > 40) {
            // Temperatura muito quente: alerta máximo
            if (tempo_inatividade > TEMPO_ALERTA1 / 2 || tempo_joystick_inativo > TEMPO_ALERTA1 / 2) {
                nivel_alerta = 3;  // Emergência
            }
        } else if (temperatura > 30) {
            // Temperatura quente: alerta acelerado
            if (tempo_inatividade > TEMPO_ALERTA2 / 2 || tempo_joystick_inativo > TEMPO_ALERTA2 / 2) {
                nivel_alerta = 2;  // Alerta Médio
            } else if (tempo_inatividade > TEMPO_ALERTA1 / 2 || tempo_joystick_inativo > TEMPO_ALERTA1 / 2) {
                nivel_alerta = 1;  // Alerta Leve
            }
        } else if (temperatura < 15) {
            // Temperatura fria: alerta desacelerado
            if (tempo_inatividade > TEMPO_ALERTA3 * 2 || tempo_joystick_inativo > TEMPO_ALERTA3 * 2) {
                nivel_alerta = 3;  // Emergência
            } else if (tempo_inatividade > TEMPO_ALERTA2 * 2 || tempo_joystick_inativo > TEMPO_ALERTA2 * 2) {
                nivel_alerta = 2;  // Alerta Médio
            } else if (tempo_inatividade > TEMPO_ALERTA1 * 2 || tempo_joystick_inativo > TEMPO_ALERTA1 * 2) {
                nivel_alerta = 1;  // Alerta Leve
            }
        } else {
            // Temperatura normal: alerta padrão
            if (tempo_inatividade > TEMPO_ALERTA3 || tempo_joystick_inativo > TEMPO_ALERTA3) {
                nivel_alerta = 3;  // Emergência
            } else if (tempo_inatividade > TEMPO_ALERTA2 || tempo_joystick_inativo > TEMPO_ALERTA2) {
                nivel_alerta = 2;  // Alerta Médio
            } else if (tempo_inatividade > TEMPO_ALERTA1 || tempo_joystick_inativo > TEMPO_ALERTA1) {
                nivel_alerta = 1;  // Alerta Leve
            }
        }

        // Exibe todas as informações no display
        exibir_informacoes(adc_read(), adc_read(), tempo_joystick_inativo, nivel_alerta, ambiente_quente, temperatura);

        // Aciona os alertas
        iniciar_alerta(nivel_alerta);

        sleep_ms(100); // Delay para evitar uso excessivo do processador
    }

    return 0;
}