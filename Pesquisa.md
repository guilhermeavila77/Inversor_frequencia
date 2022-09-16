#  Arduino-Atmel-sPWM

####  Implementação de modulação PWM em arduino


##  Introdução

Pesquisa realizada sobre como implementar um PWM em arduino com o objetivo de idealizar o funcionamento de um inversor de frequência

##  Breve Teoria

Basicamente a função de realizar a modulação PWM é realizar o controle da tenção de um circuito através de um acionamento 
e desacionamento dos componentes, criando uma senoide.

Vamos endereçar os registradores no chip atmel, bem como usar interrupções para que os cabeçalhos <avr/io.h> e <avr/interrupt.h> sejam necessários. A partir daí, temos dois arrays que possuem um sinal semi-senoidal inserido em

``` c
void  configuração (){
    // Registre a inicialização, veja a folha de dados para mais detalhes.
    TCCR1A = 0b10100010;
       /*10 clear na correspondência, definido em BOTTOM para compA.
         10 claro na partida, definido em BOTTOM para compB.
         00
         10 WGM1 1:0 para forma de onda 15.
       */
    TCCR1B = 0b00011001;
       /*000
         11 WGM1 3:2 para forma de onda 15.
         001 sem pré-escala no balcão.
       */
    TIMSK1 = 0b00000001;
       /*0000000
         1 Ativação de interrupção do sinalizador TOV1.
       */
```
Aqui o registro do temporizador foi inicializado. Se você estiver interessado nos detalhes, pode consultar o datasheet do ATMEGA328p, mas por enquanto o importante é que configuramos um PWM para dois pinos e ele chama uma rotina de interrupção para cada período do PWM.
``` c
    ICR1 = 1600; // Período para cristal de 16 MHz, para uma frequência de comutação de 100 KHz para 200 subdivisões por ciclo de onda senoidal de 50 Hz.
    sei(); // Habilita interrupções globais.
    DDRB = 0b00000110; // Configura PB1 e PB2 como saídas.
    pinMode(13,OUTPUT);
}
```
ICR1 é outro registrador que contém o comprimento do contador antes do reset, pois não temos pré-escala em nosso clock , isso define o período do PWM para 1600 clocks . Então habilitamos as interrupções. Em seguida, os dois pinos são definidos como saídas, o motivo pelo qual pinMode () não é usado é porque os pinos podem mudar em diferentes arduinos, eles também podem mudar em diferentes micros Atmel, porém você está usando um arduino com 328p, então este código vai funcionar. Por fim , pinMode () é usado para definir o pino 13 como saída, usaremos isso posteriormente como trigger para o osciloscópio, porém não é necessário.
``` c
void  loop (){; / *Não faça nada. . . . para sempre!* /}
```
Nada é implementado no loop.
``` c
ISR (TIMER1_OVF_vect){
    static int num;
    char estático;
    // muda o ciclo de trabalho a cada período.
    OCR1A = lookUp1[num];
    OCR1B = lookUp2[num];
    
    if(++num >= 200){ // Pré-incrementa num então verifique se está abaixo de 200.
       numero = 0; //Redefinir num.
       trig = trig^0b00000001;
       digitalWrite (13,trig);
     }
}
```
Esta rotina de serviço de interrupção é chamada a cada período do PWM, e a cada período o ciclo de trabalho é alterado. Isso é feito alterando o valor nos registros OCR1A e OCR1B para o próximo valor da tabela de pesquisa, pois esses registros mantêm os valores de comparação que definem os pinos de saída como baixos quando atingidos conforme a figura.

Portanto em cada período os registradores OCR1x são carregados com o próximo valor de suas tabelas de consulta usando num para apontar para o próximo valor no array, pois cada período num é incrementado e verificado se está abaixo de 200, caso não esteja abaixo 200 in é redefinido para 0. As outras duas linhas envolvendo trig e digitalWrite existem duas alternando um pino como um gatilho para um osiloscópio e não afeta o código sPWM.

###  sPWM_generate_lookup_table

O restante desta seção discute algumas modificações neste código, ou seja, podemos gerar a tabela de pesquisa no início do código, o benefício disso é alterar a frequência de comutação, bem como a frequência sPWM. O código para isso pode ser encontrado na pasta sPWM_generate_lookup_table. O início do código fica assim:
``` c
# inclui  < avr/io.h >
# inclui  < avr/interrupt.h >
# define  SinDivisions (200) // Subdivisões da onda sisusoidal.
estático  int microMHz = 16 ; // Frequência do micro clock
static  int freq = 50 ;     // Frequência senoidal
 período int longo  estático ;   // Período de PWM em ciclos de clock.
static  unsigned  int lookUp[SinDivisions];
 caractere estático theTCCR1A = 0b10000010 ; // variável para TCCR1A
void  configuração (){
  temperatura dupla; //Variável dupla para <math. h > funções.
  
  period = microMHz*1e6/freq/SinDivisions;// Período de PWM em ciclos de clock
  
  for(int i = 0; i < SinDivisions/2; i++){ // Gerando a tabela de consulta.
    temp = sin (i *2* M_PI/SinDivisions)*period;
    lookUp[i] = (int)(temp+0,5); // Arredonda para inteiro.    
  }
```
Observe que apenas a primeira metade da onda senoidal é gerada, devido à forma como este código implementa o sPMW onde cada um dos dois pinos são responsáveis ​​por diferentes metades do sinal, apenas metade da onda senoidal é necessária. No entanto, requer uma modificação na rotina de serviço de interrupção.
``` c
ISR(TIMER1_OVF_vect){
    static int num;
    int estático delay1;
    char estático;
    
    if(delay1 == 1){/*atraso de um período porque o tempo alto carregado nos valores OCR1A:B é armazenado em buffer, mas pode ser desconectado imediatamente pelo TCCR1A. */
      theTCCR1A ^= 0b10100000;// Alterna conectar e desconectar as saídas de comparação A e B.
      TCCR1A = oTCCR1A;
      atraso1 = 0; // Reinicializar atraso1
    } else if(num >= SinDivisions/2){
      numero = 0; //Redefinir numero
      atraso1++;
      trig ^=0b00000001;
      digitalWrite(13,trig);
    }
    // muda o ciclo de trabalho a cada período.
    OCR1A = lookUp[num];
    OCR1B = lookUp[num];
    num++;
}
```
