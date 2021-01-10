#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"


#define GPIO_PA0_U0RX 0x00000001
#define GPIO_PA1_U0TX 0x00000401
#define GPIO_PA4_U3RX 0x00001001
#define GPIO_PA5_U3TX 0x00001401

#define Message_Length 70

#define TimetoWait 50

unsigned char obtainedLength;
unsigned char ardMInd = 0;
uint32_t millis=0;
uint32_t time=0;
uint32_t ui32Period;
uint32_t ui32SysClkFreq;
unsigned char crcCounter = 22;
unsigned char Mess_bitArray[(Message_Length-22)*8];
unsigned char Message[Message_Length];
unsigned char MsgFromMega[Message_Length];
unsigned char Trial_Message[4] ={'T','E','S','T'};
unsigned char SRC_Add[6] = {0x10,0x1A,0xB6,0x83,0x34,0xAA}; 		// Source Address: 00:1A:B6:03:34:AA in HEX
unsigned char DEST_Add[6] = {0xCC,0x1A,0xB6,0x83,0x34,0xAA};		// Destination Address: CC:1A:B6:03:34:AA in HEX



char MakeCRC_CHECK(unsigned char *BitArr)
{
  char Bit_Length= (((obtainedLength-1)*8) +7);
	static char Res[8];                                 // CRC Result
	char CRC[7];
	char crcVar = 0;
	int  i;
	char DoInvert;
	for (i = 0; i<7; ++i)  CRC[i] = 0;                    // Init before calculation
	for (i = 0; i<Bit_Length; ++i)
	{
		DoInvert = (BitArr[i]) ^ CRC[6];         // XOR required?
		CRC[6] = CRC[5];
		CRC[5] = CRC[4];
		CRC[4] = CRC[3];
		CRC[3] = CRC[2] ^ DoInvert;
		CRC[2] = CRC[1];
		CRC[1] = CRC[0];
		CRC[0] = DoInvert;
	}
	for (i = 0; i < 7; ++i)
		crcVar = crcVar |( CRC[i] << i);
	return(crcVar);
}
bool checkCRC()
{
  char l=0;
  char res;
  for (char k = 22; k < (obtainedLength); k++)
  { 
    for (char c = 0; c < 8; c++)
    {
      Mess_bitArray[l] = ((MsgFromMega[k] >> (7-c)) & 1);
      l++;
    }
  }
  res = MakeCRC_CHECK(Mess_bitArray);
  return res;	
}

char MakeCRC(unsigned char *BitArr)
{
	static char Res[8];                                 // CRC Result
	char CRC[7];
	char crcVar = 0;
	int  i;
	char DoInvert;
	for (i = 0; i<7; ++i)  CRC[i] = 0;                    // Init before calculation
	for (i = 0; i<((crcCounter-22)*8); ++i)
	{
		DoInvert = (BitArr[i]) ^ CRC[6];         // XOR required?
		CRC[6] = CRC[5];
		CRC[5] = CRC[4];
		CRC[4] = CRC[3];
		CRC[3] = CRC[2] ^ DoInvert;
		CRC[2] = CRC[1];
		CRC[1] = CRC[0];
		CRC[0] = DoInvert;
	}
	for (i = 0; i < 7; ++i)
		crcVar = crcVar |( CRC[i] << i);
	return(crcVar);
}

void CRC()
{
	char l=0;
	for (char k = 22; k < (crcCounter); k++)
		for (char c = 0; c < 8; c++)
		{
			Mess_bitArray[l] = ((Message[k] >> (7-c)) & 1);
			l++;
		}			
	Message[crcCounter+1] = MakeCRC(Mess_bitArray);                                    // Calculate CRC
}

void Initialize_Message()
{
	unsigned char i = 0;
	// Add Preamble
	// First 7 Bytes =>> 0b10101010 = 0xAA
	// Eighth Byte =>> 0b10101011 = 0xAB
	for(i = 0; i < 7 ; i++)
		Message[i] = 0xAA;
	Message[7] = 0xAB;
	
	// Add Destination Address
	for(i = 8; i < 14 ; i++)
		Message[i] = DEST_Add[i-8];
	
	// Add Source Address
	for(i = 14 ; i < 20 ; i++)
		Message[i] = SRC_Add[i-14];
	
	Message[20] = 0xBB; // Type byte
	Message[21] = 0xBB; // Type byte
}


// Clear the previous message and keep preamble, Destination Address & Source Address
void Clear_Message()
{
	for(int i = crcCounter; i < Message_Length; i++)
		Message[i] = 0;
}

void send()
{
	CRC();
	for(int m = 0; m <(crcCounter+2)  ; m++)
	{
		//SysCtlDelay(480000);
		UARTCharPut(UART3_BASE,Message[m]);
		//UARTCharPut(UART0_BASE,Trial_Message[m]);
	}
	crcCounter = 22;
	Clear_Message();
}

void 	()
{
	// Configure System Clock
	ui32SysClkFreq = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);	
	// Enable UART0 & UART3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	
	// Configure UART0 RX & TX
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	
	// Configure UART3 RX & TX
	GPIOPinConfigure(GPIO_PA4_U3RX);
	GPIOPinConfigure(GPIO_PA5_U3TX);
	
	// Configure UART Pins
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_4 | GPIO_PIN_5);
	
	// Set UART Baud Rate to be 115200
	UARTConfigSetExpClk(UART0_BASE, ui32SysClkFreq, 115200,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
	UARTConfigSetExpClk(UART3_BASE, ui32SysClkFreq, 115200,(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));	
}

void Print(char CHAR)
{
	UARTCharPut(UART0_BASE,CHAR);
}
void PrintHEX(char CHAR)
{
	if((CHAR>>4)>9)
		UARTCharPut(UART0_BASE,((CHAR>>4)+55));
	else
		UARTCharPut(UART0_BASE,((CHAR>>4)+'0'));
	if((CHAR&0x0F)>9)
		UARTCharPut(UART0_BASE,((CHAR&0x0F)+55));
	else
		UARTCharPut(UART0_BASE,((CHAR&0x0F)+'0'));
}	
void TIMER0A_Handler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	// Read the current state of the GPIO pin and
	// write back the opposite state
	millis++;
	if(GPIOPinRead(GPIO_PORTN_BASE, GPIO_PIN_1))
	{
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
	}
	else
	{
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 2);
}
}

void Initialize_Timer()
{	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1);
	
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	ui32Period = ui32SysClkFreq/1000;
	
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
	IntEnable(INT_TIMER0A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	
	IntMasterEnable();
	TimerEnable(TIMER0_BASE, TIMER_A);
	
}
int main(void)
{	
	Initialize_Serial();
	Initialize_Message();
	Initialize_Timer();
	/*Print('T');Print('E');Print('S');Print('T');
	PrintHEX(0x9A);
	PrintHEX(0xA9);
	PrintHEX(0x0F);
	PrintHEX(0xF0);*/
	char Temp = 0 ;
	/*	Write your code here  */
		
	while (1)
	{
		if (UARTCharsAvail(UART3_BASE)) 
		{
			time = millis;
			while(1)
			{
				if(UARTCharsAvail(UART3_BASE))
				{    
					MsgFromMega[ardMInd] = UARTCharGet(UART3_BASE); 
					ardMInd++;
				}	    
				if((millis-time)>TimetoWait) 
				{
					obtainedLength = ardMInd;
					break;
				}
			}
		}
		if(ardMInd != 0)
		{
			ardMInd =0;  
			// Add Preamble
			// First 7 Bytes =>> 0b10101010 = 0xAA
			// Eighth Byte =>> 0b10101011 = 0xAB
			
			for(char o =0; o <20 ; o++)
				UARTCharPut(UART0_BASE,'=');
			Print('\n');Print('\r');
			Print('F');Print('r');Print('a');Print('m');Print('e');Print(' ');Print('R');Print('e');Print('c');Print('e');Print('i');Print('v');Print('e');Print('d');Print(' ');Print('!');
			Print('\n');Print('\r');
			Print('P');Print('r');Print('e');Print('a');Print('m');Print('b');Print('l');Print('e');Print(':');			
			Print('\n');Print('\r');
			for(char i = 0; i < 8 ; i++)
			{
				PrintHEX(MsgFromMega[i]);
				UARTCharPut(UART0_BASE,' ');
			}
			// Add Destination Address
			UARTCharPut(UART0_BASE,'\n');
			UARTCharPut(UART0_BASE,'\r');
			Print('D');Print('E');Print('S');Print('T');Print(' ');Print('M');Print('A');Print('C');Print(':');
			UARTCharPut(UART0_BASE,'\n');
			UARTCharPut(UART0_BASE,'\r');
			for(char i = 8; i < 14 ; i++)
			{
				PrintHEX(MsgFromMega[i]);
				if(i!=13)
				{
					UARTCharPut(UART0_BASE,' ');
					UARTCharPut(UART0_BASE,':');
					UARTCharPut(UART0_BASE,' ');
				}
			}
			// Add Source Address
			Print('\n');Print('\r');
			Print('S');Print('R');Print('C');Print(' ');Print('M');Print('A');Print('C');Print(':');
			Print('\n');Print('\r');
			for(char i = 14 ; i < 20 ; i++)
			{
				PrintHEX(MsgFromMega[i]);
				if(i!=19)
				{
					UARTCharPut(UART0_BASE,' ');
					UARTCharPut(UART0_BASE,':');
					UARTCharPut(UART0_BASE,' ');
				}
			}
			// Print type bits
			Print('\n');Print('\r');
			Print('T');Print('Y');Print('P');Print('E');Print(':');
			Print('\n');Print('\r');
			for(char i = 20 ; i < 22 ; i++)
			{
				PrintHEX(MsgFromMega[i]);
				if(i!=21)
				{
					UARTCharPut(UART0_BASE,' ');
					UARTCharPut(UART0_BASE,':');
					UARTCharPut(UART0_BASE,' ');
				}
			}
			Print('\n');Print('\r');
			Print('M');Print('e');Print('s');Print('s');Print('a');Print('g');Print('e');Print(':');
			Print('\n');Print('\r');
			for(char i = 22 ; i < (obtainedLength-1) ; i++)
			{
				Print(MsgFromMega[i]);
			}
			Print('\n');Print('\r');
			Print('C');Print('R');Print('C');Print(':');Print('\n');Print('\r');
			PrintHEX(MsgFromMega[obtainedLength-1]);
			MsgFromMega[obtainedLength-1] = MsgFromMega[obtainedLength-1] << 1;
			for(int y = 0 ; y < Message_Length ; y++ )
				MsgFromMega[y] = 0;
			if(!checkCRC()) {Print('\n');Print('\r');Print('C');Print('H');Print('K');Print('D');Print('\n');Print('\r');}
			else {Print('E');Print('R');Print('R');Print('\n');Print('\r');}
		}

		if (UARTCharsAvail(UART0_BASE)) 
		{
			Temp = UARTCharGet(UART0_BASE);
			UARTCharPut(UART0_BASE,Temp);
			if (Temp == 13)
			{
				UARTCharPut(UART0_BASE,'\n');
				UARTCharPut(UART0_BASE,'\r');
				send();
				continue;
			}				
			Message[crcCounter]=Temp;
			crcCounter++;
			if(crcCounter == ( Message_Length))
				send();
		}
	}
}