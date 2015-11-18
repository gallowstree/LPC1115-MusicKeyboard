// Plantilla de código para la práctica de laboratorio #4 de Microprocesadores
// Eduardo Corpeño
// Universidad Galileo, FISICC

#include "LPC11xx.h"

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


static const double natural_tones[] = {TONE_C1, TONE_D1, TONE_E1, TONE_F1, TONE_G1, TONE_A1, TONE_B1};
static const double sharp_tones[] = {TONE_C1_SHARP, TONE_D1_SHARP, TONE_E1_SHARP, TONE_F1_SHARP, TONE_G1_SHARP, TONE_A1_SHARP, TONE_B1_SHARP};

char octave = 4;
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
	initPins();
	initPWM();
	
	LPC_GPIO0->DIR &= ~MASK(2);
	

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
		playAllNotes();
	}
}
// *******************************ARM University Program Copyright © ARM Ltd 2013*************************************   
