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
#define TONE_C1_SHARP 35.0*2
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

//Para leer teclas
#define KEY_A_DOWN ~LPC_GPIO0->DATA & MASK(10)
#define KEY_B_DOWN ~LPC_GPIO0->DATA & MASK(11)
#define KEY_G_DOWN ~LPC_GPIO0->DATA & MASK(8)
#define KEY_F_DOWN ~LPC_GPIO0->DATA & MASK(2)
#define KEY_E_DOWN ~LPC_GPIO0->DATA & MASK(3)
#define KEY_D_DOWN ~LPC_GPIO0->DATA & MASK(7)
#define KEY_C_DOWN ~LPC_GPIO0->DATA & MASK(9)
#define KEY_SHARP_DOWN ~LPC_GPIO0->DATA & MASK(6)

//Paseo del cero pue'
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
static const char* natural_note_names[] = {"DO", "RE", "MI", "FA", "SOL", "LA", "SI"}; 
static const char* sharp_note_names[] = {"DO#", "RE#", "FA", "FA#", "SOL#", "LA#", "DO+"}; 
static uint8_t special_pos_char[] = {254};
static uint8_t special_line1_char[] = {128};
static uint8_t special_line2_char[] = {192};

int numRow = 1;
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

void clearScreen()
{
	UARTSendString((uint8_t*)"                                ");
}

void printOctave()
{
	uint8_t oct[1];	
	UARTSend(special_pos_char, 1);
	UARTSend(special_line2_char, 1);
	UARTSendString((uint8_t*)"OCTAVA: ");
	oct[0] = 48 + octave;
	UARTSend(oct, 1);
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

void STFU()
{
	LPC_TMR32B1->TCR &= ~MASK(0); //Detener cuenta	
	LPC_TMR32B1->MR3 = 0; //hacer match freq veces por segundo
	LPC_TMR32B1->MR0 = 1000; //duty cycle del 50%  
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

int lastNote = -2;
int wasSharp = 0;
int get_frequency()
{
	const double* selected_tones = KEY_SHARP_DOWN ? sharp_tones : natural_tones;
	const char** note_names = KEY_SHARP_DOWN ? sharp_note_names : natural_note_names;
	int note = -1;
	
	if (KEY_C_DOWN)
		note = NOTE_C;
	else if (KEY_D_DOWN)
		note = NOTE_D;
	else if (KEY_E_DOWN)
		note = NOTE_E;
	else if (KEY_F_DOWN)
		note = NOTE_F;
	else if (KEY_G_DOWN)
		note = NOTE_G;
	else if (KEY_A_DOWN)
		note = NOTE_A;
	else if (KEY_B_DOWN)
		note = NOTE_B;
  
	
	if (note != -1)
	{
		if (lastNote != note || (KEY_SHARP_DOWN) != wasSharp) 
		{
			clearScreen();
			UARTSend(special_pos_char, 1);
			UARTSend(special_line1_char, 1);
			UARTSendString((uint8_t*)"NOTA: ");
			UARTSendString((uint8_t*)note_names[note]);
			printOctave();
			lastNote = note;
			wasSharp = KEY_SHARP_DOWN;
		}
		return round_(MASK(octave) * selected_tones[note]);
	}
	else
	{
		lastNote = -2;
		clearScreen();
		printOctave();
		return 0;
	}
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
		if(row3 || row2 || row1)
		{ 
			delay(1000000);
		}
		if(row4)
		{ 
			octave--;
			if (octave == 0) octave = 5;
			delay(1000000);
		}
	} else if(col2){
		if(row3 || row2 || row1 || row4)
		{ 
			delay(1000000);
		}
	} else if(col3)
	{		
		if(row3 || row2 || row1)
		{ 
			delay(1000000);
		}
		if(row4)
		{ 			
			octave++;
			if (octave == 6) octave = 1;
			delay(1000000);
		}
	}
}

void readNumKeypad()
{
	if(numRow == 5){ numRow = 1; }
		setRows(numRow);
		numRow++;
		checkCols();
}

void showSplashScreen()
{
	UARTSendString((uint8_t*)"A CLOCKWORK     KEYBOARD");
	delay(7000000);
	clearScreen();
	UARTSend(special_pos_char, 1);
	UARTSend(special_line1_char, 1);
	UARTSendString((uint8_t*)"DANIEL RODRIGUEZ");
	delay(7000000);
	clearScreen();
	UARTSend(special_pos_char, 1);
	UARTSend(special_line1_char, 1);
	UARTSendString((uint8_t*)"PAOLO SOLOMBRINO");
	delay(7000000);
	clearScreen();
	UARTSend(special_pos_char, 1);
	UARTSend(special_line1_char, 1);
	UARTSendString((uint8_t*)"ALEJANDRO       ALVARADO");
	delay(7000000);
	clearScreen();
}

int main()
{	
	initPins();
	initPWM();
	
	LPC_GPIO0->DIR &= ~MASK(8);
	LPC_GPIO0->DIR &= ~MASK(10);
	LPC_GPIO0->DIR &= ~MASK(11);
	LPC_GPIO0->DIR &= ~MASK(6);
	LPC_GPIO0->DIR &= ~MASK(2);
	LPC_GPIO0->DIR &= ~MASK(3);
	LPC_GPIO0->DIR &= ~MASK(7);
	LPC_GPIO0->DIR &= ~MASK(9);
	
	LPC_IOCON->SWDIO_PIO1_3 = (LPC_IOCON->SWDIO_PIO1_3 & ~0x7) | 0x1;
	
	//salidas del teclado matricial
	LPC_GPIO0->DIR |= MASK(5);
	LPC_GPIO1->DIR |= MASK(3);
	LPC_GPIO1->DIR |= MASK(5);
	LPC_GPIO1->DIR |= MASK(9);
	//entradas del teclado matricial
	LPC_GPIO1->DIR &= ~MASK(2);
	LPC_GPIO1->DIR &= ~MASK(4);
	LPC_GPIO1->DIR &= ~MASK(8);
	
	delay(9000000);
	GPIOInit(); 
	
  UARTInit(9600);

	showSplashScreen();

	while(1)
	{
		int faux = 0;
		int freq = get_frequency();
		faux = freq;
		
		if (freq > 0)
			setFrequency(freq);
		while (faux == freq)
		{				
				faux = get_frequency();
				readNumKeypad();			
		}
		STFU();
		readNumKeypad();		
	}

}
// *******************************ARM University Program Copyright © ARM Ltd 2013*************************************   
