#ifndef _BSP_INIT
#define _BSP_INIT


#include "stm32f10x.h"


typedef struct{
	
	volatile uint16_t recv;      //单次接收数据长度

	volatile uint8_t  done;

	volatile uint8_t BufferChangeFlag;
	
} Buffer_status;

void BSP_Init(uint32_t baud);
Buffer_status DMA_Buffer_PUSH(void);

int uart2_dma_fifo_getByte(uint8_t *ch);
int uart2_dma_fifo_getData(uint8_t *dst, uint16_t datalen);
void UART2_SendByte(uint8_t b);
void UART2_SendBuf(const uint8_t *pbuf, uint16_t len);


#endif

