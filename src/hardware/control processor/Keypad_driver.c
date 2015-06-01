/* ***********************

	keypad driver

**************************/


#include<stdint.h>
#include<stdbool.h>
#include "inc/tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"	

uint8_t ui8PinData=1;
int32_t button=0;
int i,j;
uint8_t col_scan;
uint8_t coloumn[]={GPIO_PIN_1,GPIO_PIN_2,GPIO_PIN_3};
uint8_t row[]={GPIO_PIN_1,GPIO_PIN_2,GPIO_PIN_3};
void keypressed(uint8_t,uint8_t);


void keypad_init()
{
//uint8_t col_scan;
//int i,j=0;

SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1);
GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1, 0x00);
//GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1);

GPIOPinTypeGPIOInput(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);// column PF1,2,3.
GPIOPinTypeGPIOOutput(GPIO_PORTL_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);// row PL1,2,3.

GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD_WPU);//COLOUMN=INPUT
GPIOPadConfigSet(GPIO_PORTL_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD_WPD);//ROW=OUTPUT


	//GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_3, 0x08);
	//GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x00);	
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x0E);	
	GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x00);	


}
int main()
{

keypad_init();

while(1)
{
	for(i=0;i<3;i++)//selsct row
	{
		GPIOPinWrite(GPIO_PORTL_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, 0x0E);	
		GPIOPinWrite(GPIO_PORTL_BASE, row[i], 0x00);
		for(j=0;j<3;j++)
		{
			col_scan=GPIOPinRead(GPIO_PORTF_BASE,coloumn[j]);
			if(col_scan==0)
			{
				//GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);
				keypressed(i,j);//corresponding row and coloumn
				
			}
		}	
	}
}

return 0;
}

void keypressed(uint8_t r,uint8_t c)
{
	if(r==0&&c==0)
	{	
		//1
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);
	}
	if(r==0&&c==1)
	{
		//2
				//GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);
	}
	if(r==0&&c==2)
	{
		//3
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);
	}
	if(r==1&&c==0)
	{
		//4
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);
	}
	if(r==1&&c==1)
	{
		//5
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);
	}
	if(r==1&&c==2)
	{
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);
		//6
	}
	if(r==2&&c==0)
	{
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);
		//7
	}
	if(r==2&&c==1)
	{
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);
		//8
	}
	if(r==2&&c==2)
	{
		//9
				GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);
	}
}

