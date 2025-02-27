#include "ws2818b.pio.h"

#define NUM_LEDS 25
#define PINO_LED 7

// Estrutura de cor GRB
struct corLED_t
{
    uint8_t verde, vermelho, azul;
};
typedef struct corLED_t corLED_t;
typedef corLED_t matrizLED_t; 

matrizLED_t matriz[NUM_LEDS];
PIO controladorPIO;
uint estadoMaquina;

void configurarBotao(uint gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}


void configurarLED(uint gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, 0);
}

void inicializarPIO(uint pino)
{
    uint deslocamento = pio_add_program(pio0, &ws2818b_program);
    controladorPIO = pio0;

    estadoMaquina = pio_claim_unused_sm(controladorPIO, false);
    if (estadoMaquina < 0)
    {
        controladorPIO = pio1;
        estadoMaquina = pio_claim_unused_sm(controladorPIO, true);
    }

    ws2818b_program_init(controladorPIO, estadoMaquina, deslocamento, pino, 800000.f);

    // Zera todos os LEDs da matriz.
    for (uint i = 0; i < NUM_LEDS; i++)
    {
        matriz[i].vermelho = 0;
        matriz[i].verde = 0;
        matriz[i].azul = 0;
    }
}

void definirCorLED(uint indice, uint8_t r, uint8_t g, uint8_t b)
{
    matriz[indice].vermelho = r;
    matriz[indice].verde = g;
    matriz[indice].azul = b;
}

/**
 * Reseta os LEDs, desligando todos.
 */
void limparMatriz()
{
    for (uint i = 0; i < NUM_LEDS; i++)
        definirCorLED(i, 0, 0, 0);
}

/**
 * Atualiza a matriz de LEDs com os valores armazenados no buffer.
 */
void atualizarMatriz()
{
    for (uint i = 0; i < NUM_LEDS; i++)
    {
        pio_sm_put_blocking(controladorPIO, estadoMaquina, matriz[i].verde);
        pio_sm_put_blocking(controladorPIO, estadoMaquina, matriz[i].vermelho);
        pio_sm_put_blocking(controladorPIO, estadoMaquina, matriz[i].azul);
    }
    sleep_us(100);
}

int calcularIndice(int x, int y)
{
    return (y % 2 == 0) ? 24 - (y * 5 + x) : 24 - (y * 5 + (4 - x));
}


void exibirMatriz(int matrizLED[5][5][3], int tempo, float brilho)
{
    for (int linha = 0; linha < 5; linha++)
    {
        for (int coluna = 0; coluna < 5; coluna++)
        {
            int indice = calcularIndice(linha, coluna);
            definirCorLED(indice, matrizLED[coluna][linha][0] * brilho,
                          matrizLED[coluna][linha][1] * brilho,
                          matrizLED[coluna][linha][2] * brilho);
        }
    }
    atualizarMatriz();
    sleep_ms(tempo);
    limparMatriz();
}

