#include "stm32f10x.h"
#include "led.h"
#include "delay.h"
#include "sys.h"
#include "oled.h"
#include "string.h"
#include "ds18b20.h"
#include "string.h"
#include "usart.h"
#include <stdio.h>

#include "BSP_Init.h"
#include "bootloader_config.h"



//u8 temp;
//u8 humi;
static Buffer_status buf_statu;
static uint8_t dst[256];



int main(void)
{
    delay_init(72);
    BSP_Init(UART2_BAUD);

//    for (uint8_t k = 0; k < 5; k++)
//    {
//        UART2_SendBuf(dst, sizeof(dst));
//        delay_ms(200);
//    }

    while(1)
    {
			
			
//				for (uint8_t k = 0; k < 5; k++)
//				{
//					UART2_SendBuf(dst, sizeof(dst));
//					delay_ms(200);
//				}
        buf_statu = DMA_Buffer_PUSH();

        int n = uart2_dma_fifo_getData(dst, sizeof(dst));
        if (n > 0)
        {
            UART2_SendBuf(dst, (uint16_t)n);
        }

        

    }
}
