//Bibliotecas
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

Adafruit_PCD8544 display = Adafruit_PCD8544( 48, 51, 50, 53, 52);

#define divSenoide (100)
#define Valternada A7 // Pino de entrada da amostra do sinal AC de saída
#define Vcontinua A6 // Pino de entrada da amostra do sinal CC de entrada (bateria12V)
#define Ccontinua A5 // Pino de entrada da amostra do sinal CC de corrente deentrada
#define N 30

// Declaração de variáveis
float voltageAC;
float voltageCC;
float currentCC;

long soma1 = 0.0;
long soma2 = 0.0;
long soma3 = 0.0;

int valstempo1[N];
int valstempo2[N];
int valstempo3[N];

int TporAmp = 100;
int tensao = 5000;
int ACSoffset = 2500;

int Estado = 0;

static int freeqArduino = 16;
static int frequencia = 60;
static long int Periodo;
static unsigned int lookUp[divSenoide];

void setup() {
    Serial.begin(9600);
    display.begin(); 
    display.setRotation(2);
    display.setContrast(60);
    display.clearDisplay();
    display.display();
    analogReference(EXTERNAL);
    double tempo;
    Periodo = freeqArduino * 1e6 / frequencia / divSenoide;
    for (int i = 0; i < divSenoide / 2; i++) { 
    tempo = sin(i * 2 * M_PI / divSenoide) * Periodo;
    lookUp[i] = (int)(tempo + 0.5); 
}

ISR(TIMER1_OVF_vect) {
    static int num;
    if (num >= divSenoide / 2) {
    num = 0; // Reset num
    Estado ^= 1;
    PORTB ^= 0b10000000;

    }
    if (Estado == 0) {
        OCR1A = 0; 
        OCR1B = 0; 
        OCR3A = lookUp[num]; 
        OCR3B = lookUp[num]; 
        num ++;
    }
    if (Estado == 1) {
        OCR1A = lookUp[num];
        OCR1B = lookUp[num]; 
        OCR3A = 0; 
        OCR3B = 0; 
        num++;
    }
}
ISR(TIMER3_OVF_vect) {
}

//Calculo da media da corrente continua

long movingAvaregeCCCurr(int port) {
    long sumtempo = 0.0;
    for (int i = N - 1; i > 0; i--) {
        valstempo3[i] = valstempo3[i - 1];
    }
    valstempo3[0] = analogRead(port);
    delay(5);
    for (int i = 0; i < N; i++) {
        sumtempo = sumtempo + valstempo3[i];
    }
    return sumtempo / N;
}

//Calculo da media da corrente alternada
long movingAvarageacVolt(int port) {
    long sumtempo = 0.0;
    for (int i = N - 1; i > 0; i--) {
        valstempo1[i] = valstempo1[i - 1];
    }
    valstempo1[0] = analogRead(port);
    delay(5);
    for (int i = 0; i < N; i++) {
        sumtempo = sumtempo + valstempo1[i];
    }
    return sumtempo / N;
}
//Calculo da media da tensão continua

long movingAvarageCCVolt(int port) {
    long sumtempo = 0.0;
    for (int i = N - 1; i > 0; i--) {
        valstempo2[i] = valstempo2[i - 1];
    }
    valstempo2[0] = analogRead(port);
    delay(5);
    for (int i = 0; i < N; i++) {
        sumtempo = sumtempo + valstempo2[i];
    }
    return sumtempo / N;
}

// Inicialização do registrador.
TCCR1A = 0b10110010;
TCCR3A = 0b10110010; 

TCCR1B = 0b00011001;
TCCR3B = 0b00011001;

TIMSK1 = 0b00000001;
TIMSK3 = 0b00000001;

ICR1 = Periodo;
ICR3 = Periodo; 

sei();

DDRB |= 0b11100000; 
DDRE |= 0b00011000;

void loop() {
    voltageAC = (movingAvarageacVolt(Valternada) / 1024.0) * 5;
    voltageAC = voltageAC * (264.08 / 5);
    voltageCC = (movingAvarageCCVolt(Vcontinua) / 1024.0) * 5;
    voltageCC = voltageCC * (15.5 / 5);
    currentCC = ((movingAvaregeCCCurr(Ccontinua) / 1024.0) * 5) * 1000;
    currentCC = ((currentCC - ACSoffset) / TporAmp);


    display.setTextSize(1);
    display.clearDisplay();
    display.setTextColor(BLACK, WHITE); 
    display.setCursor(0, 0); 
    display.print("Inversor CC/AC"); 
    display.drawFastHLine(0, 9, 84, BLACK); 
    display.setCursor(0, 10);
    display.setTextColor(WHITE, BLACK);
    display.print(" AC OUTPUT ");
    display.setTextColor(BLACK, WHITE);
    display.setCursor(15, 20);
    display.print(voltageAC, 1);
    display.print(" Vac ");
    display.drawFastHLine(0, 29, 84, BLACK);
    display.setCursor(0, 30);
    display.setTextColor(WHITE, BLACK);
    display.print(" CC INPUT ");
    display.setCursor(2, 40);
    display.setTextColor(BLACK, WHITE);
    display.print(voltageCC, 1);
    display.setCursor(29, 40);
    display.print("V");
    display.setCursor(45, 40);
    display.print(currentCC, 2);
    display.print(" A");
    display.display();
    delay(100);
}