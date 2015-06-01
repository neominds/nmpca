#include<stdint.h>
#include<stdbool.h>
#include "inc/tm4c1294ncpdt.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"	

uint8_t ui8PinData=1;
int32_t button=0;
int main()
{
SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1);
GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0|GPIO_PIN_1, 0x00);
GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1);
GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_4MA,GPIO_PIN_TYPE_STD_WPU);

while(1)
{
/* on board push switch pj1 and pj2*/
if(!GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))  //Read goes low when the switch is pressed
GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x02);

if(!GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))
GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_0,0x01);

else{
GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0x00);
GPIOPinWrite(GPIO_PORTN_BASE,GPIO_PIN_0,0x00);
}
//SysCtlDelay(5000000);
}
return 0;
}
