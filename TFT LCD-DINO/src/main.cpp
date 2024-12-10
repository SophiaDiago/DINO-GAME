#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Sprite.h"

// Pines para la conexión con el TFT LCD
#define TFT_DC 7
#define TFT_CS 6
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 10
#define TFT_MISO 12

#define botonUp 20
#define botonDown 21
#define buzzerPin 9

// Parámetros para la pantalla
const int XMAX = 240;
const int YMAX = 320;
const int dinoX = 50;
int dinoY = YMAX - 96;
const int dinoYInicial = YMAX - 96;
// Va movimientos del dino
int dinoVelocidad = 0;
bool saltando = false;
bool agachado = false;
unsigned long agachadoTimer = 0; 
const long duracionAgachado = 4000; 
const int gravedad = 6;
const int alturaSalto = -40;
bool saltoExitoso = false;
const int alturaMaxima = dinoYInicial - 48;

bool todosCactusPasaron = false;
bool aveAparecio = false;
unsigned long tiempoAveEspera = 0;
const int tiempoEsperaEntreCactusYAve = 10000; //8 segundos
unsigned long tiempoAnterior = 0;
const long intervaloAnimacion = 20;

// Variables del juego
int puntaje = 0;
bool gameOver = false;


Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// Estructura para la ave
struct Ave {
    int x;
    int y;
    bool activo;
    const uint8_t *sprite;

    Ave(int x_pos, int y_pos)
        : x(x_pos), y(y_pos), activo(false), sprite(AVE1) {}
};

// Inicialización de la ave
int aveY = dinoY - 28; // a la cabeza del dino
Ave ave(XMAX, aveY);
int aveSpeed = 30;
unsigned long aveTimer = 0;
unsigned long aveIntervalo = 90000;


// Estructura para los cactus
struct Cactus {
    int x;
    int y;
    bool activo;
    const uint8_t *sprite;

    Cactus(int x_pos, int y_pos, const uint8_t *sprite_ptr)
        : x(x_pos), y(y_pos), activo(false), sprite(sprite_ptr) {}
};


// Inicialización de los cactus
int cactusY = YMAX - 64; // a los pies del dino
Cactus cactus[3] = {
    Cactus(XMAX, cactusY, CACTUS1),
    Cactus(XMAX, cactusY, CACTUS2),
    Cactus(XMAX, cactusY, CACTUS3)
};

int cactusSpeed = 50;
unsigned long cactusTimers[3] = {0, 0, 0};
unsigned long cactusIntervalos[3] = {0, 40000, 90000}; // intervalos entre los cactus (estre mas separados mejor)

// Declaración de funciones
void dibujarDino();
void actualizarCactusYAve();
void mostrarPuntaje();
void iniciarSalto();
void actualizarSalto();
void iniciarAgachado();
void actualizarAgachado();
void dibujarPiso();
void mostrarGameOver();
void reproducirSonidoVictoria();
void reproducirSonidoGAMEOVER();
bool detectarColision();


// Configuración inicial
void setup() {
    screen.begin(9600);
    screen.fillScreen(ILI9341_BLACK);
    screen.setTextColor(ILI9341_WHITE);
    screen.setTextSize(3);
    screen.setCursor(ILI9341_TFTWIDTH / 2 - 36, 55);
    screen.println("DINO");
    screen.setCursor(ILI9341_TFTWIDTH / 2 - 36, 90);
    screen.println("GAME");

    pinMode(botonUp, INPUT);
    pinMode(botonDown, INPUT);
    pinMode(buzzerPin, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(botonUp), iniciarSalto, RISING);
    attachInterrupt(digitalPinToInterrupt(botonDown), iniciarAgachado, RISING);
}

void loop() {
    if (gameOver) return;
    
    actualizarSalto();
    actualizarAgachado();
    actualizarCactusYAve();

    unsigned long tiempoActual = millis();

    // la animación del dinosaurio solo se actualiza cada intervaloAnimacion 20 ms
    if (tiempoActual - tiempoAnterior >= intervaloAnimacion) {
        tiempoAnterior = tiempoActual;
        dibujarDino();
    }
    dibujarPiso();


    if (detectarColision()) {
        gameOver = true;
        mostrarGameOver();
        return;
    }

    mostrarPuntaje();
    delay(5);
}

// Función para dibujar el dinosaurio
void dibujarDino() {
    screen.fillRect(dinoX, dinoY, 64, 64, ILI9341_BLACK);
    if (agachado) {
        screen.drawBitmap(dinoX, dinoY + 32, DINO4, 64, 32, ILI9341_WHITE);
    } else {
        screen.drawBitmap(dinoX, dinoY, DINO1, 64, 64, ILI9341_WHITE);
    }
}

void iniciarSalto() {
    if (!saltando && !gameOver) {
        saltando = true;
        dinoVelocidad = alturaSalto;
        saltoExitoso = false;
    }
}

void actualizarSalto() {
    if (saltando) {
        screen.fillRect(dinoX, dinoY, 64, 64, ILI9341_BLACK);
        dinoY += dinoVelocidad;
        dinoVelocidad += gravedad;

        if (dinoY < alturaMaxima) {
            dinoY = alturaMaxima;
            dinoVelocidad = gravedad;
        }

        if (dinoY >= dinoYInicial) {
            dinoY = dinoYInicial;
            saltando = false;
            dinoVelocidad = 0;

            // Aumenta el puntaje si salta y no choca
            if (!detectarColision()) {
                puntaje += 5;
                saltoExitoso = true;
                reproducirSonidoVictoria();
            }
        }

        screen.drawBitmap(dinoX, dinoY, DINO1, 64, 64, ILI9341_WHITE);
    }
}

void iniciarAgachado() {
    if (!agachado && !saltando && !gameOver) {
        agachado = true;
        agachadoTimer = millis();
        dibujarDino();
    }
}

void actualizarAgachado() {
    if (agachado) {
        if (millis() - agachadoTimer > duracionAgachado) {
            agachado = false;
            dibujarDino();
            
            // Incrementar el puntaje al pasar de DINO4 a DINO1
            puntaje += 5;
            reproducirSonidoVictoria(); // Sonido al aumentar puntaje por esquivar el ave
        }
    }
}


void actualizarCactusYAve() {
    unsigned long tiempoActual = millis();

    // Mostrar el ave primero
    if (!ave.activo && !todosCactusPasaron) {
        ave.activo = true;
        ave.x = XMAX;  // Colocamos el ave al inicio de la pantalla
    }

    // Actualizar el movimiento del ave
    if (ave.activo) {
        //screen.fillRect(ave.x - aveSpeed * 2, ave.y, 56 + aveSpeed * 4, 36, ILI9341_BLACK);
        screen.fillRect(0, ave.y - 1, XMAX, 38, ILI9341_BLACK);
        screen.drawBitmap(ave.x, ave.y, ave.sprite, 56, 36, ILI9341_WHITE);
        ave.x -= aveSpeed;
       

        // Si el ave sale de la pantalla, desactívala y permite que los cactus aparezcan
        if (ave.x < -120) {
            ave.activo = false;
            todosCactusPasaron = true; 
        }
    }

    // Mostrar los cactus después de que el ave haya salido completamente
    if (todosCactusPasaron) {
        for (int i = 0; i < 3; i++) {
            // Si el cactus no está activo, actívalo con el tiempo original que tenía
            if (!cactus[i].activo && (tiempoActual - cactusTimers[i] >= cactusIntervalos[i])) {
                cactus[i].activo = true;
                cactus[i].x = XMAX + (i * 80); // Mantener la separación establecida entre los cactus
            }

            // Actualizar el movimiento de los cactus
            if (cactus[i].activo) {
                screen.fillRect(cactus[i].x, cactus[i].y, 32, 32, ILI9341_BLACK);
                cactus[i].x -= cactusSpeed;
                screen.drawBitmap(cactus[i].x, cactus[i].y, cactus[i].sprite, 32, 32, ILI9341_WHITE);

                // Si el cactus sale de la pantalla, desactívalo
                if (cactus[i].x < -32) {
                    cactus[i].activo = false;
                }
            }
        }

        // Verificar si todos los cactus han pasado y reiniciar el ciclo
        bool todosInactivos = true;
        for (int i = 0; i < 3; i++) {
            if (cactus[i].activo) {
                todosInactivos = false;
                break;
            }
        }

        // Si todos los cactus han pasado, reinicia el ciclo y espera para mostrar el ave nuevamente
        if (todosInactivos) {
            todosCactusPasaron = false;
            aveAparecio = false;
        }
    }
}


// Función para detectar colisiones
bool detectarColision() {
    for (int i = 0; i < 3; i++) {

       // verifica si las coordenadas de dinoX, dinoY están superponiéndose con las del cactus que este
        if (cactus[i].activo) {
            if (dinoX + 64 > cactus[i].x && dinoX < cactus[i].x + 32 &&
                dinoY + 64 > cactus[i].y && dinoY < cactus[i].y + 32) {
                return true;
            }
        }
    }
    return false;
}


// "GAME OVER"
void mostrarGameOver() {
    reproducirSonidoGAMEOVER();
    screen.fillRect(0, 50, XMAX, 80, ILI9341_BLACK);
    screen.setTextColor(ILI9341_WHITE);
    screen.setTextSize(3);
    screen.setCursor(XMAX / 2 - 36, 55);
    screen.print("GAME");
    screen.setCursor(XMAX / 2 - 36, 90);
    screen.print("OVER");
}

// Piso
void dibujarPiso() {
    screen.drawLine(0, YMAX - 32, XMAX, YMAX - 32, ILI9341_WHITE);
}

// Puntaje
void mostrarPuntaje() {
    screen.fillRect(XMAX - 100, 10, 120, 20, ILI9341_BLACK);
    screen.setCursor(XMAX - 110, 10);
    screen.setTextSize(2);
    screen.print("Score: ");
    screen.print(puntaje);
}

// Sonido de victoria
void reproducirSonidoVictoria() {
    int melody[] = {1000, 1600}; 

    for (int i = 0; i < 4; i++) {
        tone(buzzerPin, melody[i]);
        delay(250); 
    }
    noTone(buzzerPin);
}

// Sonido de derrota
void reproducirSonidoGAMEOVER() {
    int melody[] = {800, 600, 400}; 

    for (int i = 0; i < 3; i++) {
        tone(buzzerPin, melody[i]);
        delay(400); 
    }
    noTone(buzzerPin);
}
