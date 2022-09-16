// Codigo responsavel por operar as saidas do microcontrolador realizando a modulação PWM

//Bibliotecas utilizadas
#include <avr/interrupt.h>
#define SinDivisions (100)

//Variaveis referentes a frequencia
static int frequenciamicro = 16; 
static int frequencia = 60; 
static long int periodo; 
static unsigned int lookUp[SinDivisions];
static char theTCCR1A = 0b10000010;

void setup(){
  double temp; 
  periodo
 = frequenciamicro*1e6/frequencia/SinDivisions;
  for(int i = 0; i < SinDivisions/2; i++){ 
  temp = sin(i*2*M_PI/SinDivisions)*periodo
;
  lookUp[i] = (int)(temp+0.5); 
 }
 
TCCR1A = theTCCR1A;

TCCR1B = 0b00011001;

TIMSK1 = 0b00000001;

ICR1 = periodo; 
 
 sei();
 DDRB = 0b00000110; 
 pinMode(13, OUTPUT);
}
void loop(){;/*não fazer nada*/}

ISR(TIMER1_OVF_vect){
  static int num;
  static int delay1;
  static char trig;
  int sensorValue = analogRead(A0);
  
  if(delay1 == 1){
    theTCCR1A ^= 0b10100000;
    TCCR1A = theTCCR1A;
    delay1 = 0;
    } else if(num >= SinDivisions/2){
    num = 0;
    delay1++;
    trig ^=0b00000001;
    digitalWrite(13,trig);
  }
  OCR1A = OCR1B = lookUp[num]+sensorValue*2;
  num++;
}
