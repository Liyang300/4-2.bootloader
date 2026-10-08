#include "stm32f10x.h"
#include "led.h"
#include "delay.h"
#include "sys.h"
#include "oled.h"
#include "string.h"
#include "ds18b20.h"
#include "string.h" 	
//#include "usart.h"
#include <stdio.h>


u8 temp;
u8 humi;

int main(void)
{
	unsigned char p[16]=" ";

	short temperature = 0; 				//温度值
	delay_init(72);
	LED_Init();
	OLED_Init();
	delay_ms(50);
	OLED_Clear();
	OLED_ShowChinese(0,0,0,16,1);
	OLED_ShowChinese(16,0,1,16,1);
	OLED_ShowChar(40,0,':',16,1);
	while(DS18B20_Init())	//DS18B20初始化	
	{
		OLED_ShowString(0,0,"DS18B20 Error",16,1);
		delay_ms(200);
		OLED_ShowString(60,0,"        " ,16,1);	
		delay_ms(200);
	}
	delay_ms(1000);

  while(1)
  {

		temperature = DS18B20_Get_Temp();
		sprintf((char*)p,"%4.1f    ",(float)temperature/10);
		OLED_ShowString(60,0,p ,16,1);
		
		delay_ms(100);
//    LED_On();
//		delay_ms(500);
//		LED_Off();
//		delay_ms(500);
  }
}

