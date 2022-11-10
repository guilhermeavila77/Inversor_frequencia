/***************************************************************************
              Inversor de tensão DC / AC controlado pelo Arduino
****************************************************************************
  -- IDE do Arduino Versão 1.8.3
  -- Autor do codigo base do display: Limor Fried/Ladyada, Adafruit Industries.
         https://github.com/adafruit/Adafruit-GFX-Library
         https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
  -- Autor codigo inicial sPWM: Kurt Hutten
         https://github.com/Terbytes/Arduino-Atmel-sPWM
  -- Autor versão com o Mega e display, modulanto os 4 MOSFETs: Eduardo Avelar
  -- Blog: easytromlabs.com
  -- email: contato@easytromlabs.com

  -- Fevereiro, 2018
****************************************************************************
*****************************************************************************
                      Conexões entre Hardwares:
             Graphic LCD Pin --------------- Arduino Pin
                 1-RST       ----------------  52
                 2-CE        ----------------  53
                 3-DC        ----------------  50
                 4-DIN(MOSI) ----------------  51
                 5-CLK       ----------------  48
                 6-VCC       ----------------  3,3V
                 7-LED       ----------------  GND
                 8-GND       ----------------  GND
****************************************************************************/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
                                          //CLK, DIN, DC, CE, RST//

// Defines
#define SinDivisions (100)           // Sub divisoes da onda senoidal
#define ACVOLTPIN A7                 // Pino de entrada da amostra do sinal AC de saída
#define DCVOLTPIN A6                 // Pino de entrada da amostra do sinal DC de entrada (bateria 12V)
#define DCCURRPIN A5                 // Pino de entrada da amostra do sinal DC de corrente de entrada
#define N 30

// Declaração de variáveis
int valsTemp1[N];
int valsTemp2[N];
int valsTemp3[N];
long sumTemp1 = 0.0;
long sumTemp2 = 0.0;
long sumTemp3 = 0.0;
int mVperAmp = 100;
int Vcc = 5000;
int ACSoffset = 2500;
static int microMHz = 16;                         // Frequencia do microcontrolador
static int freq = 60;                             // Frequandia da senoide
static long int period;                           // Periodo 16,6 ms
static unsigned int lookUp[SinDivisions];
int state = 0;
float voltageAC;
float voltageDC;
float currentDC;

void setup() {
  Serial.begin(9600);
  analogReference(EXTERNAL);

  double temp;                                   
  period = microMHz * 1e6 / freq / SinDivisions;          // (2666.67) Period of PWM in clock cycles

  for (int i = 0; i < SinDivisions / 2; i++) {            // Gera a tabela ou vetor
    temp = sin(i * 2 * M_PI / SinDivisions) * period;
    lookUp[i] = (int)(temp + 0.5);                        // Arredonda para um numero inteiro
  }
  // Inicialização do registrador.
  TCCR1A = 0b10110010; //varible for TCCR1A
  TCCR3A = 0b10110010; //varible for TCCR3A
  /*10 Comp A, apaga no valor de comparação, (Modo não invertido
    11 Comp B, ativa no valor de comparação, (Modo invertido)
    00
    10 WGM1 1:0 for waveform 14.
  */
  TCCR1B = 0b00011001;
  TCCR3B = 0b00011001;
  /*000
    11 WGM1 3:2 for waveform 14.
    001 no prescale on the counter.
  */
  TIMSK1 = 0b00000001;
  TIMSK3 = 0b00000001;
  /*0000000
    1 TOV1 Flag interrupt enable.
  */
  ICR1   = period;   // Periodo para 16MH de cristal, 100 subdivisões e 60Hz de frequencia da onda de saída
  ICR3   = period;   // Periodo para 16MH de cristal, 100 subdivisões e 60Hz de frequencia da onda de saída

  sei();             // Enable global interrupts.

  // Set outputs pins.
  DDRB |= 0b11100000; // Arduino MEGA - Set PB5 (D11-PWM), PB6 (D12-PWM), PB7 (D13-LED) como saídas
  DDRE |= 0b00011000; // Arduino MEGA - Set PE3 (D5-PWM), PE4 (D2-PWM) como saídas
}

void loop() {
  voltageAC = (movingAvarageacVolt(ACVOLTPIN) / 1024.0) * 5;
  voltageAC = voltageAC * (264.08 / 5);

  voltageDC = (movingAvaragedcVolt(DCVOLTPIN) / 1024.0) * 5;
  voltageDC = voltageDC * (15.5 / 5);

  currentDC = ((movingAvaregedcCurr(DCCURRPIN) / 1024.0) * 5) * 1000;
  currentDC = ((currentDC - ACSoffset) / mVperAmp);
}

//***************************************************************************//
// Função que calcula a media móveol da leitura feita no canal A7 - AC VOLT  //
//***************************************************************************//
long movingAvarageacVolt(int port) {

  long sumTemp = 0.0;

  for (int i = N - 1; i > 0; i--) {
    valsTemp1[i] = valsTemp1[i - 1];
  }
  valsTemp1[0] = analogRead(port);
  delay(5);

  for (int i = 0; i < N; i++) {
    sumTemp = sumTemp + valsTemp1[i];
  }

  return sumTemp / N;
}

//***************************************************************************//
// Função que calcula a media móveol da leitura feita no canal A6 - DC VOLT  //
//***************************************************************************//
long movingAvaragedcVolt(int port) {

  long sumTemp = 0.0;

  for (int i = N - 1; i > 0; i--) {
    valsTemp2[i] = valsTemp2[i - 1];
  }
  valsTemp2[0] = analogRead(port);
  delay(5);

  for (int i = 0; i < N; i++) {
    sumTemp = sumTemp + valsTemp2[i];
  }

  return sumTemp / N;
}

//***************************************************************************//
// Função que calcula a media móveol da leitura feita no canal A5 - DC CURRE //
//***************************************************************************//
long movingAvaregedcCurr(int port) {

  long sumTemp = 0.0;

  for (int i = N - 1; i > 0; i--) {
    valsTemp3[i] = valsTemp3[i - 1];
  }
  valsTemp3[0] = analogRead(port);
  delay(5);

  for (int i = 0; i < N; i++) {
    sumTemp = sumTemp + valsTemp3[i];
  }

  return sumTemp / N;
}
///////////////////////////////////////////////////////////////////////////////
ISR(TIMER1_OVF_vect) {
  static int num;

  if (num >= SinDivisions / 2) {
    num = 0;                // Reset num
    state ^= 1;

    PORTB ^= 0b10000000;  // Arduino MEGA - Faz um togle no pino PB0 - D8 (LED) 
                          //sem afetar os outros pinos utilizando uma XOR
  }

  if (state == 0) {
    OCR1A = 0;    // D11 - Q1 -> OFF
    OCR1B = 0;    // D12 - Q2 -> ON (Complemento do D11)

    OCR3A = lookUp[num]; // D5 - Q3 -> PWM
    OCR3B = lookUp[num]; // D2 - Q4 -> PWM Complemento
    num ++;
  }

  if (state == 1) {
    OCR1A = lookUp[num]; // D11 - Q1 -> PWM
    OCR1B = lookUp[num]; // D12 - Q2 -> PWM Complemento

    OCR3A = 0;   // D5 - Q3 -> OFF
    OCR3B = 0;   // D2 -> ON
    num++;
  }
}

ISR(TIMER3_OVF_vect) {
}
  //-- Apenas como entendimento da matemática nos pinos -----//
  //PORTB ^= 0b00000001; // Faz um togle no pino PB0 - (D8) sem afetar os outros bits utilizando uma XOR
  //PORTB |= 0b00000001; // Escreve no pino PB0 - (D8) sem afetar os outros pinos
  //PORTB &= 0b11111110; // Apaga o bit PB0 - (D8) sem afetar os  outros pinos
