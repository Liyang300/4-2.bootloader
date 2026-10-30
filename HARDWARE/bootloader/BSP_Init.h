#ifndef _BSP_INIT
#define _BSP_INIT


#include "stm32f10x.h"


typedef struct{
	
	volatile uint16_t recv;      //单次接收数据长度

	volatile uint8_t  done;

	volatile uint8_t BufferChangeFlag;
	
} Buffer_status;


Buffer_status DMA_Buffer_PUSH(void);




#endif