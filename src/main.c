/*
 * main implementation: use this 'C' sample to create your own application
 *
 */


#include "S32K142.h" /* include peripheral declarations S32K142 */

//Motor a Pasos

unsigned long secuenciaPasoDoble[4] = {0xC00, 0x600, 0x300, 0x900};      //Secuencia Paso Doble
unsigned long secuenciaPasoSimple[4] = {0x800, 0x400, 0x200, 0x100};     //Secuencia Paso Simple
unsigned char i = 0;													 //Posición de la secuencia
unsigned char sentido = 0;												 //Sentido del motor
unsigned long retardo = 0;												 // Tiempo de retardo en milisegundos (Velocidad del Motor)
unsigned char ascii;													 //valor ingresado

void LPUART1_RxTx_IRQHandler (void) {						//Función de la interrupción del UART

	ascii = LPUART1 -> DATA;

	if (ascii != 13){										//Numero 13 es el enter en ascii
		retardo = (retardo * 10) + (ascii - 0x30);			//Agregar valor de la terminal en la variable retardo
	}else{
		LPTMR0 -> CSR = 0;									    //Apaga el reloj
		LPTMR0 -> CMR = retardo - 1;							//Establecer el valor de referencia para comprarar las iteraciones del reloj. Se determina cuentas-1. (C47-1408)
		LPTMR0 -> CSR = 1 + (1<<6);								//Inicializa el LPTMR0, solo se habilita TEN bit 0 confg "1". Ademas habilita la interrupcion del LPTMR0 con TIE bit 6 confg "1" (C47-1404)
		retardo = 0;
	}
}

void LPTMR0_IRQHandler (void) {								//Función de la interrupción del LPTMR0
	LPTMR0 -> CSR |= (1<<7);    							//Apagar la bandera TCF en bit 7 conf W1C para reiniciar el timer (C47-1404)
    if (sentido==0) PTD->PDOR=secuenciaPasoSimple[(i++)%4]; //Sigue la secuencia del arreglo. Dirección 1. Se suma el valor de la posición del arreglo hasta el modulo 4 por el numero de valores en el arreglo.
    else PTD->PDOR=secuenciaPasoSimple[(i--)%4];			//Sigue la secuencia del arreglo. Dirección 2. Se resta el valor de la posición del arreglo hasta el modulo 4 por el numero de valores en el arreglo.
}

void PORTC_IRQHandler (void)								//Función de la interrupción del botón
{
    if (((PORTC->PCR[12]) & (1<<12))==0) {
    	PORTC->PCR[12]|=(1<<24);							//Apaga la bandera
    	sentido^=1;											//Cuando presión el botón, cambia el sentido
    }

}

int main(void){

	PCC -> PCCn[PCC_PORTD_INDEX] = 0x40000000;				//Habilitar Reloj Puerto D, solo se habilita CGC bit 30 (C29-633)				//Habilitar Reloj Puerto D, solo se habilita CGC bit 30 (C29-633)
	PORTD -> PCR[8] = 0x00000100;                     		//Habilitar Puerto D como GPIO con el MUX bit 8-10 confg 001 1<<8 (C12-198)
	PORTD -> PCR[9] = 0x00000100;                     		//Habilitar Puerto D como GPIO con el MUX bit 8-10 confg 001 1<<8 (C12-198)
	PORTD -> PCR[10] = 0x00000100; 							//Habilitar Puerto D como GPIO con el MUX bit 8-10 confg 001 1<<8 (C12-198)
	PORTD -> PCR[11] = 0x00000100; 							//Habilitar Puerto D como GPIO con el MUX bit 8-10 confg 001 1<<8 (C12-198)

	PCC -> PCCn[PCC_PORTC_INDEX] = 0x40000000;        		//Habilitar Reloj Puerto C, solo se habilita CGC bit 30 (C29-631)
	PORTC -> PCR[12] = (1<<8) + (10<<16);                   //Habilitar Puerto C como GPIO con el MUX bit 8-10 confg 001 1<<8 (C12-198). Botón
	PORTC -> PCR[6] = (2<<8);								//Habilitar Puerto C como UART con el MUX bit 8-10 confg 010 2<<8 (C12-198 Excel). Guía rápida (Receiver)
	PORTC -> PCR[7] = (2<<8);								//Habilitar Puerto C como UART con el MUX bit 8-10 confg 010 2<<8 (C12-198 Excel). Guía rápida (Transmiter)

	PTD -> PDDR = (15<<8);                    	 			//Configurar pin 8,9,10,11 como salidas con "1" confg 0111 7<<4 + 0110 3<<9 (C13-218)
	PTD -> PDOR = 0;										//Iniciar con los pines apagados (C13-213)

	PCC -> PCCn[PCC_LPTMR0_INDEX] = (1<<30);         		//Habilitar Reloj LPTMR0, solo se habilita CGC bit 30 (C29-626)
	LPTMR0 -> PSR = 5;                             	 		//Elegir base de tiempo con el MUX bit 0-1 confg "01" 1K = 1ms, ver diagrama (C27-551). Ademas elegir la opcion de bypass con bit 2 confg "1" (C47-1406). Escribe 5 = 101
	LPTMR0 -> CSR = 0;                        				//Empieza desactivado

	PCC -> PCCn[PCC_LPUART1_INDEX] = (1<<30) + (3<<24);  	//Habilitar Reloj UART, se habilita CGC bit 30 y en PCS bit 24-26 conf 011 para elegir FIRCDIV2 (C29-646), ver diagramas (C27-536) y (C27-546)
	SCG -> FIRCDIV = (1<<8);								//Elegir el preescaner del FIRCDIV2 bit 8-10 confg 001 1<<8 (C28-581)
	LPUART1 -> BAUD = 312;									//Establecer la velocidad de comunicacion en SBR bit 012 (C51-1544). SBR = CLK UART / 16 * BAUD Rate. CLK UART = 48 MHz, BAUD Rate = 9600 bps, Rx va 16 veces mas rapido que Tx.
															//NOTA: No sobre pasar el 5% de error. SBR Teorico = 312.5, SBR Real = 312. Error = 0.16%
	LPUART1 -> CTRL = (3<<18) + (1<<21);					//(C51-1551)

	S32_NVIC -> ISER[58/32] |= 1<<(58 % 32);				//Habilitar la interrupcion del LPTMR0. 58 -- ID de la interrupcion (Excel). |= Asegurarse que no cambien los demas bits

	S32_NVIC -> ISER[33/32] |= 1<<(33 % 32);				//Habilita a la interrupcion del UART. 33 -- ID de la interrupcion (Excel). |= Asegurarse que no cambien los demas bits

	S32_NVIC -> ISER[58/32] |= 1<<(61 % 32);				//Habilita a la interrupcion del Botón. 61 -- ID de la interrupcion (Excel). |= Asegurarse que no cambien los demas bits

    for(;;) {}

	return 0;
}
