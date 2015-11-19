// Plantilla de código para la práctica de laboratorio #4 de Microprocesadores
// Eduardo Corpeño
// Universidad Galileo, FISICC

#include "type.h"
#include "LPC11xx.h"
#include "uart.h"
#include "gpio.h"

#define MASK(x) (1UL << (x))
//Frecuencias base para tonos naturales
#define TONE_C1 32.70*2
#define TONE_D1 36.71*2
#define TONE_E1 41.20*2
#define TONE_F1 43.65*2
#define TONE_G1 49.00*2
#define TONE_A1 55.00*2
#define TONE_B1 61.74*2

//Frecuencias base para tonos sostenidos
#define TONE_C1_SHARP 34.65*2
#define TONE_D1_SHARP 38.89*2
#define TONE_E1_SHARP 43.65*2
#define TONE_F1_SHARP 46.25*2
#define TONE_G1_SHARP 51.91*2
#define TONE_A1_SHARP 58.27*2
#define TONE_B1_SHARP 65.41*2

//Para indexar los arreglos
#define NOTE_C 0
#define NOTE_D 1
#define NOTE_E 2
#define NOTE_F 3
#define NOTE_G 4
#define NOTE_A 5
#define NOTE_B 6

#define row1On  LPC_GPIO1->DATA |=  (MASK(3))
#define row1Off LPC_GPIO1->DATA &= ~(MASK(3))
#define row2On  LPC_GPIO0->DATA |=  (MASK(5))
#define row2Off LPC_GPIO0->DATA &= ~(MASK(5))
#define row3On  LPC_GPIO1->DATA |=  (MASK(9))
#define row3Off LPC_GPIO1->DATA &= ~(MASK(9))
#define row4On  LPC_GPIO1->DATA |=  (MASK(5))
#define row4Off LPC_GPIO1->DATA &= ~(MASK(5))
#define row1 !(LPC_GPIO1->DATA & MASK(3))
#define row2 !(LPC_GPIO0->DATA & MASK(5))
#define row3 !(LPC_GPIO1->DATA & MASK(9))
#define row4 !(LPC_GPIO1->DATA & MASK(5))
#define col1 !(LPC_GPIO1->DATA & MASK(4))
#define col2 !(LPC_GPIO1->DATA & MASK(2))
#define col3 !(LPC_GPIO1->DATA & MASK(8))

static const double natural_tones[] = {TONE_C1, TONE_D1, TONE_E1, TONE_F1, TONE_G1, TONE_A1, TONE_B1};
static const double sharp_tones[] = {TONE_C1_SHARP, TONE_D1_SHARP, TONE_E1_SHARP, TONE_F1_SHARP, TONE_G1_SHARP, TONE_A1_SHARP, TONE_B1_SHARP};

char octave = 4;
char* firstCol[1],  * firstRow[1],* oct[1]; 


void initPins()
{
	LPC_SYSCON->SYSAHBCLKCTRL |= (MASK(6) | MASK(16)); //reloj a GPIO e IOCON
	LPC_IOCON->R_PIO1_1 |= (MASK(0) | MASK(1)); //R_PIO1_1 FUNC bits en 0x3,  CT32B1_MAT0
}

void initPWM()
{
	LPC_SYSCON->SYSAHBCLKCTRL |= MASK(10); //CT32B1
	LPC_TMR32B1->MR3 = 1000;
	LPC_TMR32B1->MR0 = 1000;
	LPC_TMR32B1->MCR = MASK(10); //Resetear cuenta al hacer match con MR3
	LPC_TMR32B1->TC = 0; //Contador en 0
	LPC_TMR32B1->PWMC |= MASK(0); //PWM en el canal 0 (controlar CT32B1_MAT0)
	LPC_TMR32B1->TCR |= MASK(0); //Iniciar cuenta	
}

void setFrequency(int freq)
{
	int div = 48000000/freq; //el reloj va a 48 MHz, dividimos este intervalo dentro de los hertz que queremos generar
	LPC_TMR32B1->TCR &= ~MASK(0); //Detener cuenta	
	LPC_TMR32B1->MR3 = div; //hacer match freq veces por segundo
	LPC_TMR32B1->MR0 = div >> 1; //duty cycle del 50%  
	LPC_TMR32B1->TC = 0; //Contador en 0
	LPC_TMR32B1->TCR |= MASK(0); //Iniciar cuenta	
}
void delay(int dly)
{
	while(dly--);
}

void setRows(int currRow){
	if(currRow == 1){
		row1Off; row2On; row3On; row4On;	
	} else if(currRow == 2){
		row1On; row2Off; row3On; row4On;	
	}else if(currRow == 3){
		row1On; row2On; row3Off; row4On;	
	}else if(currRow == 4){
		row1On; row2On; row3On; row4Off;	
	}
}

void checkCols(){
	if(col1){
		if(row1){ 
			UARTSendString((uint8_t*)"1");
			delay(1000000);
		}
		if(row2){ 
			UARTSendString((uint8_t*)"4");
			delay(1000000);
		}
		if(row3){ 
			UARTSendString((uint8_t*)"7");
			delay(1000000);
		}
		if(row4){ 
			UARTSendString((uint8_t*)"*");
			octave--;
			if (octave == 0) octave = 5;
			delay(1000000);
		}
	} else if(col2){
		if(row1){ 
			UARTSendString((uint8_t*)"2");
			delay(1000000);
		}
		if(row2){ 
			UARTSendString((uint8_t*)"5");
			delay(1000000);
		}
		if(row3){ 
			UARTSendString((uint8_t*)"8");
			delay(1000000);
		}
		if(row4){ 
			UARTSendString((uint8_t*)"0");
			delay(1000000);
		}
	} else if(col3){
		if(row1){ 
			UARTSendString((uint8_t*)"3");
			delay(1000000);
		}
		if(row2){ 
			UARTSendString((uint8_t*)"6");
			delay(1000000);
		}
		if(row3){ 
			UARTSendString((uint8_t*)"9");
			delay(1000000);
		}
		if(row4){ 
			UARTSendString((uint8_t*)"#");
			octave++;
			if (octave == 6) octave = 1;
			delay(1000000);
		}
	}
}

void refreshLCD(){
	/**oct[0] = 48 + octave;
	UARTSend((uint8_t *)firstCol,1);
	UARTSend((uint8_t *)firstRow,1);
	UARTSend((uint8_t *)oct,1);*/
	UARTSendString((uint8_t*)"K");
}

int round_(double d)
{
	return (int)(d < 0 ? (d - 0.5) : (d + 0.5));
}

void playAllNotes()
{
	int note, oct;
	for (oct = 1; oct <6; oct++ )
	{
		for (note = NOTE_C; note <= NOTE_B; note++)
		{
			double freq = natural_tones[note] * MASK(oct);
			int intFreq = round_(freq);
			setFrequency(intFreq);
			delay(1000000);
			
		}
	}
}

int main()
{	
	int play = 0;
	int numRow = 1;
	*firstRow[0] = 128;
	*firstCol[0] = 254;
	initPins();
	initPWM();
	
	LPC_GPIO0->DIR &= ~MASK(2);
	
	LPC_IOCON->SWDIO_PIO1_3 = (LPC_IOCON->SWDIO_PIO1_3 & ~0x7) | 0x1;
	//LPC_IOCON->R_PIO1_2 = (LPC_IOCON->R_PIO1_2 & ~0x7) | 0x1;
	
	//salidas del teclado matricial
	LPC_GPIO0->DIR |= MASK(5);
	LPC_GPIO1->DIR |= MASK(3);
	LPC_GPIO1->DIR |= MASK(5);
	LPC_GPIO1->DIR |= MASK(9);
	//entradas del teclado matricial
	LPC_GPIO1->DIR &= ~MASK(2);
	LPC_GPIO1->DIR &= ~MASK(4);
	LPC_GPIO1->DIR &= ~MASK(8);
	
	
	GPIOInit();
 

		delay(6000000);
    UARTInit(9600);
    UARTSendString((uint8_t*)"TOC TOC QUE CUAAAAANTO?");

	while(1)
	{
		/*if (play)
		{
			setFrequency(round_(natural_tones[NOTE_A] * MASK(3)));
			delay(1000000);
		}
		if (~LPC_GPIO0->DATA & MASK(2))
		{
			play = 1;
		}*/
		//playAllNotes();
		

		/*funciones pa leer el teclado*/
		if(numRow == 5){ numRow = 1; }
		setRows(numRow);
		numRow++;
		checkCols();
		refreshLCD();
	}
}

// *******************************ARM University Program Copyright © ARM Ltd 2013*************************************   
